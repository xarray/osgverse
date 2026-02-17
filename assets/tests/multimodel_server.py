# pip install Pillow flask
import hashlib, json, os, argparse, io, time, struct, threading
from typing import Dict, Callable, Any, Optional, Tuple
from flask import Flask, request, jsonify
from functools import wraps
from PIL import Image
from dataclasses import dataclass
from enum import IntEnum

try:
    from multiprocessing import shared_memory
    SHARED_MEMORY_AVAILABLE = True
except ImportError:
    SHARED_MEMORY_AVAILABLE = False
    print("Warning: shared_memory not available")
UPLOAD_FOLDER = './uploads'
MAX_CONTENT_LENGTH = 16 * 1024 * 1024 * 1024  # 16GB
STREAM_THRESHOLD = 10 * 1024 * 1024  # 10MB
CHUNK_SIZE = 8192  # 8KB

os.makedirs(UPLOAD_FOLDER, exist_ok=True)
app = Flask(__name__)
app.config['MAX_CONTENT_LENGTH'] = MAX_CONTENT_LENGTH
chunked_uploads: Dict[str, Dict] = {}

# ================== Shared memory =================

class LockStatus(IntEnum):
    IDLE = 0
    CLIENT_WRITING = 1
    SERVER_READING = 2
    PROCESSING = 3
    SERVER_WRITING = 4
    CLIENT_READING = 5
    READY = 6
    ERROR = 7

@dataclass
class ShmHeader:
    magic: int = 0x53484D45  # "SHME"
    version: int = 1
    status: int = 0
    data_size: int = 0
    buffer_size: int = 0
    data_type: int = 0       # 0=binary, 1=text, 2=image
    checksum: int = 0
    timestamp: float = 0.0
    flags: int = 0
    HEADER_SIZE = 64

    def pack(self) -> bytes:
        return struct.pack('<IIIIIIIddQ',
            self.magic, self.version, self.status, self.data_size,
            self.buffer_size, self.data_type, self.checksum,
            self.timestamp, self.flags)

    @classmethod
    def unpack(cls, data: bytes) -> 'ShmHeader':
        if len(data) < cls.HEADER_SIZE:
            raise ValueError("Invalid header size")
        unpacked = struct.unpack('<IIIIIIIddQ', data[:cls.HEADER_SIZE])
        return cls(*unpacked)

    def is_valid(self) -> bool:
        return self.magic == 0x53484D45 and self.version == 1

class SharedMemoryManager:
    def __init__(self):
        self._shm_map: Dict[str, shared_memory.SharedMemory] = {}
        self._local_locks: Dict[str, threading.Lock] = {}
        self._metadata: Dict[str, dict] = {}
        self._global_lock = threading.Lock()

    def create(self, name: str, size: int, exist_ok: bool = False) -> Tuple[bool, str]:
        try:
            with self._global_lock:
                # Close existing shm
                if name in self._shm_map:
                    if not exist_ok:
                        return False, "Shared memory already exists"
                    self.close(name)

                # Create new shm
                total_size = ShmHeader.HEADER_SIZE + size
                try:
                    shm = shared_memory.SharedMemory(create=True, size=total_size, name=name)
                except FileExistsError:
                    if exist_ok:
                        # Open existing shm
                        shm = shared_memory.SharedMemory(name=name)
                    else:
                        return False, "Shared memory already exists"

                # Iniitialize header
                header = ShmHeader(
                    status=LockStatus.IDLE,
                    buffer_size=size,
                    timestamp=time.time()
                )
                shm.buf[:ShmHeader.HEADER_SIZE] = header.pack()

                self._shm_map[name] = shm
                self._local_locks[name] = threading.Lock()
                self._metadata[name] = {
                    'created': time.time(),
                    'access_count': 0,
                    'owner': 'server'
                }
                return True, "OK"
        except Exception as e:
            return False, str(e)

    def open(self, name: str) -> Optional[shared_memory.SharedMemory]:
        try:
            with self._global_lock:
                if name not in self._shm_map:
                    shm = shared_memory.SharedMemory(name=name)
                    self._shm_map[name] = shm
                    self._local_locks[name] = threading.Lock()
                    self._metadata[name] = {'opened': time.time(), 'owner': 'client'}
                return self._shm_map.get(name)
        except Exception as e:
            print(f"Failed to open SHM {name}: {e}")
            return None

    def read_header(self, name: str) -> Optional[ShmHeader]:
        shm = self._shm_map.get(name)
        if not shm:
            return None
        try:
            header_data = bytes(shm.buf[:ShmHeader.HEADER_SIZE])
            header = ShmHeader.unpack(header_data)
            return header if header.is_valid() else None
        except Exception:
            return None

    def write_header(self, name: str, header: ShmHeader) -> bool:
        shm = self._shm_map.get(name)
        if not shm:
            return False
        try:
            shm.buf[:ShmHeader.HEADER_SIZE] = header.pack()
            return True
        except Exception as e:
            print(f"Failed to write header: {e}")
            return False

    def read_data(self, name: str, offset: int = 0, size: int = None) -> Optional[bytes]:
        shm = self._shm_map.get(name)
        if not shm:
            return None
        try:
            header = self.read_header(name)
            if not header:
                return None

            data_size = size or header.data_size
            start = ShmHeader.HEADER_SIZE + offset
            end = start + min(data_size, header.buffer_size - offset)

            return bytes(shm.buf[start:end])
        except Exception as e:
            print(f"Failed to read data: {e}")
            return None

    def write_data(self, name: str, data: bytes, offset: int = 0) -> bool:
        shm = self._shm_map.get(name)
        if not shm:
            return False
        try:
            header = self.read_header(name)
            if not header:
                return False

            if offset + len(data) > header.buffer_size:
                raise ValueError("Data exceeds buffer size")

            start = ShmHeader.HEADER_SIZE + offset
            shm.buf[start:start + len(data)] = data

            # 更新数据大小
            header.data_size = max(header.data_size, offset + len(data))
            header.timestamp = time.time()
            self.write_header(name, header)

            return True
        except Exception as e:
            print(f"Failed to write data: {e}")
            return False

    def set_status(self, name: str, status: LockStatus) -> bool:
        header = self.read_header(name)
        if not header:
            return False
        header.status = status
        header.timestamp = time.time()
        return self.write_header(name, header)

    def wait_for_status(self, name: str, target_status: LockStatus,
                       timeout: float = 30.0, poll_interval: float = 0.001) -> bool:
        start_time = time.time()
        while time.time() - start_time < timeout:
            header = self.read_header(name)
            if header and header.status == target_status:
                return True
            time.sleep(poll_interval)
        return False

    def close(self, name: str) -> bool:
        try:
            with self._global_lock:
                if name in self._shm_map:
                    self._shm_map[name].close()
                    try:
                        self._shm_map[name].unlink()
                    except:
                        pass
                    del self._shm_map[name]

                if name in self._local_locks:
                    del self._local_locks[name]

                if name in self._metadata:
                    del self._metadata[name]
            return True
        except Exception as e:
            print(f"Failed to close SHM {name}: {e}")
            return False

# global SHM manager
shm_mgr = SharedMemoryManager()

def _shm_read_operation(shm_name: str, metadata: Dict) -> Dict:
    """ read SHM from client """
    if not shm_mgr.open(shm_name):
        return {'status': 'error', 'type': 'shm', 'message': f'Failed to open SHM: {shm_name}'}
    if not shm_mgr.wait_for_status(shm_name, LockStatus.CLIENT_WRITING, timeout=5.0):
        return {'status': 'error', 'type': 'shm', 'message': 'Timeout waiting for client data'}

    shm_mgr.set_status(shm_name, LockStatus.SERVER_READING)
    data = shm_mgr.read_data(shm_name)
    header = shm_mgr.read_header(shm_name)
    if data is None:
        shm_mgr.set_status(shm_name, LockStatus.ERROR)
        return {'status': 'error', 'type': 'shm', 'message': 'Failed to read data from SHM'}

    data_type_map = {0: 'binary', 1: 'text', 2: 'image', 3: 'json'}
    actual_type = data_type_map.get(header.data_type, 'binary')
    handler = get_handler(actual_type)
    result = handler(data, metadata)

    shm_mgr.set_status(shm_name, LockStatus.IDLE)
    result['shm_operation'] = 'read'
    result['shm_name'] = shm_name
    result['data_type'] = actual_type
    return result

def _shm_write_operation(shm_name: str, data: bytes, metadata: Dict) -> Dict:
    """ write to SHM from server """
    size = metadata.get('size', len(data) if data else 1024 * 1024)
    success, msg = shm_mgr.create(shm_name, size, exist_ok=False)
    if not success:
        return {'status': 'error', 'type': 'shm', 'message': msg}
    if data:
        if not shm_mgr.write_data(shm_name, data):
            return {'status': 'error', 'type': 'shm', 'message': 'Failed to write data to SHM'}

    shm_mgr.set_status(shm_name, LockStatus.READY)
    return {
        'status': 'success', 'type': 'shm',
        'shm_operation': 'write',
        'shm_name': shm_name, 'size': size,
        'message': 'Data ready in SHM, client can read now'
    }

def _shm_bidirectional_operation(shm_name: str, metadata: Dict) -> Dict:
    """ Read from client and then write from server """
    if not shm_mgr.open(shm_name):
        return {'status': 'error', 'type': 'shm', 'message': f'Failed to open SHM: {shm_name}'}
    if not shm_mgr.wait_for_status(shm_name, LockStatus.CLIENT_WRITING, timeout=10.0):
        return {'status': 'error', 'type': 'shm', 'message': 'Timeout waiting for client input'}
    shm_mgr.set_status(shm_name, LockStatus.SERVER_READING)
    input_data = shm_mgr.read_data(shm_name)
    if input_data is None:
        shm_mgr.set_status(shm_name, LockStatus.ERROR)
        return {'status': 'error', 'type': 'shm', 'message': 'Failed to read input data'}

    shm_mgr.set_status(shm_name, LockStatus.PROCESSING)
    result_data = process_shm_data(input_data, metadata)

    header = shm_mgr.read_header(shm_name)
    if len(result_data) <= header.buffer_size:
        # overwrite input SHM
        shm_mgr.write_data(shm_name, result_data, offset=0)
        header.data_size = len(result_data)
        header.status = LockStatus.READY
        shm_mgr.write_header(shm_name, header)
        return {
            'status': 'success', 'type': 'shm',
            'shm_operation': 'bidirectional',
            'shm_name': shm_name,
            'input_size': len(input_data),
            'output_size': len(result_data),
            'message': 'Result written to same SHM'
        }
    else:
        # write to new SHM
        result_shm_name = f"{shm_name}_result"
        success, msg = shm_mgr.create(result_shm_name, len(result_data) * 2)
        if success:
            shm_mgr.write_data(result_shm_name, result_data)
            shm_mgr.set_status(result_shm_name, LockStatus.READY)
            shm_mgr.set_status(shm_name, LockStatus.ERROR)
            return {
                'status': 'success', 'type': 'shm',
                'shm_operation': 'bidirectional',
                'input_shm': shm_name,
                'output_shm': result_shm_name,
                'input_size': len(input_data),
                'output_size': len(result_data),
                'message': 'Result written to new SHM'
            }
        else:
            shm_mgr.set_status(shm_name, LockStatus.ERROR)
            return {'status': 'error', 'type': 'shm', 'message': 'Failed to create result SHM'}

# ==================== Handlers ====================

def process_shm_data(data: bytes, metadata: Dict) -> bytes:
    """ TODO: handle SHM in/out """
    result = {
        'processed': True,
        'input_size': len(data),
        'timestamp': time.time(),
    }
    return json.dumps(result).encode('utf-8')

def handle_shm(data: bytes, metadata: Dict[str, Any]) -> Dict:
    shm_name = metadata.get('shm_name')
    operation = metadata.get('operation', 'read')  # read/write/bidirectional
    if not shm_name:
        return {'status': 'error', 'type': 'shm', 'message': 'shm_name required in metadata'}
    if not SHARED_MEMORY_AVAILABLE:
        return {'status': 'error', 'type': 'shm', 'message': 'Shared memory not available on server'}

    try:
        if operation == 'read':
            return _shm_read_operation(shm_name, metadata)
        elif operation == 'write':
            return _shm_write_operation(shm_name, data, metadata)
        elif operation == 'bidirectional':
            return _shm_bidirectional_operation(shm_name, metadata)
        else:
            return {'status': 'error', 'type': 'shm', 'message': f'Unknown operation: {operation}'}
    except Exception as e:
        return {'status': 'error', 'type': 'shm', 'message': f'SHM operation failed: {str(e)}'}

def handle_text(data: bytes, metadata: Dict[str, Any]) -> Dict:
    try:
        text = data.decode('utf-8')
        ''' TODO: text handing '''

        return {
            'status': 'success',
            'type': 'text', 'size': len(data),
            'metadata': metadata
        }
    except UnicodeDecodeError:
        return {
            'status': 'error', 'type': 'text',
            'message': 'Invalid UTF-8 text data'
        }

def handle_image(data: bytes, metadata: Dict[str, Any]) -> Dict:
    try:
        image = Image.open(io.BytesIO(data))
        if image.mode != 'RGB':
            image_rgb = image.convert('RGB')
        else:
            image_rgb = image.copy()
        width, height = image.size
        format_type = image.format or 'UNKNOWN'
        ''' TODO: image handing '''

        return {
            'status': 'success',
            'type': 'image', 'size': len(data),
            'md5': hashlib.md5(data).hexdigest(),
            'metadata': metadata
        }
    except Exception as e:
        return {
            'status': 'error', 'type': 'image',
            'message': f'Image processing failed: {str(e)}'
        }

def handle_binary(data: bytes, metadata: Dict[str, Any]) -> Dict:
    ''' TODO: binary data handing '''

    return {
        'status': 'success',
        'type': 'binary', 'size': len(data),
        'md5': hashlib.md5(data).hexdigest(),
        'metadata': metadata
    }

def handle_json(data: bytes, metadata: Dict[str, Any]) -> Dict:
    try:
        decoded = data.decode('utf-8')
        json_root = json.loads(decoded)
        ''' TODO: JSON data handing '''

        return {
            'status': 'success',
            'type': 'json', 'size': len(data),
            'metadata': metadata
        }
    except (UnicodeDecodeError, json.JSONDecodeError) as e:
        return {
            'status': 'error', 'type': 'json',
            'message': f'Invalid JSON: {str(e)}'
        }

def handle_file(data: bytes, metadata: Dict[str, Any]) -> Dict:
    filename = metadata.get('filename', 'unknown')
    file_path = os.path.join(UPLOAD_FOLDER, filename)
    with open(file_path, 'wb') as f:
        f.write(data)
    ''' TODO: file data handing '''

    return {
        'status': 'success',
        'type': 'file', 'size': len(data),
        'savepath': file_path,
        'md5': hashlib.md5(data).hexdigest(),
        'metadata': metadata
    }

def handle_unknown(data: bytes, metadata: Dict[str, Any]) -> Dict:
    return {
        'status': 'error', 'type': 'unknown',
        'message': f'Unknown data type: {metadata.get("type", "undefined")}',
        'supported_types': ['text', 'image', 'binary', 'json', 'file']
    }

# ==================== 分段上传处理 ====================

def handle_chunked_upload() -> Dict:
    upload_id = request.headers.get('X-Upload-ID')
    chunk_index = request.headers.get('X-Chunk-Index', type=int)
    total_chunks = request.headers.get('X-Total-Chunks', type=int)
    data_type = request.args.get('type', 'binary')
    if not all([upload_id, chunk_index is not None, total_chunks]):
        return {'status': 'error', 'message': 'Missing chunk headers'}

    # Read current chunk
    chunk_data = request.get_data()
    if upload_id not in chunked_uploads:
        chunked_uploads[upload_id] = {
            'chunks': {},
            'total': total_chunks,
            'type': data_type,
            'metadata': {
                'filename': request.headers.get('X-Filename'),
                'content_type': request.content_type
            }
        }

    session = chunked_uploads[upload_id]
    session['chunks'][chunk_index] = chunk_data
    if len(session['chunks']) == total_chunks:
        # All chunks arrived, merge them
        complete_data = b''.join(
            session['chunks'][i] for i in range(total_chunks)
        )

        handler = get_handler(data_type)
        result = handler(complete_data, session['metadata'])
        del chunked_uploads[upload_id]

        result['upload_mode'] = 'chunked'
        result['total_chunks'] = total_chunks
        return result

    return {
        'status': 'chunk_received',
        'upload_id': upload_id,
        'chunk_index': chunk_index,
        'received': len(session['chunks']),
        'total': total_chunks
    }

def get_handler(data_type: str) -> Callable:
    handlers = {
        'text': handle_text,
        'image': handle_image,
        'binary': handle_binary,
        'json': handle_json,
        'file': handle_file,
        'shm': handle_shm,
    }
    return handlers.get(data_type, handle_unknown)

# ==================== Route ====================

@app.route('/upload', methods=['POST'])
def upload():
    """
    Query Parameters:
        type: text, image, binary, json, file
        mode: normal, chunked
    Headers (for chunked data):
        X-Upload-ID: Session ID
        X-Chunk-Index: Chunk index
        X-Total-Chunks: Total chunk count
        X-Filename: Optional filename
    """
    data_type = request.args.get('type', 'binary')
    upload_mode = request.args.get('mode', 'normal')

    # Chunked mode
    if upload_mode == 'chunked' or request.headers.get('X-Upload-ID'):
        return jsonify(handle_chunked_upload())

    # SHM type
    if data_type == 'shm':
        metadata = {
            'shm_name': request.args.get('shm_name') or request.headers.get('X-Shm-Name'),
            'operation': request.args.get('operation', 'read'),
            'size': request.args.get('size', type=int),
            'filename': request.headers.get('X-Filename'),
            'content_type': request.content_type, 'type': 'shm'
        }
        result = handle_shm(b'', metadata)
        return jsonify(result)

    # Normal mode
    try:
        if request.content_length and request.content_length > STREAM_THRESHOLD:
            # if content > 10MB, use streaming method
            data = bytearray()
            stream = request.stream
            while True:
                chunk = stream.read(CHUNK_SIZE)
                if not chunk:
                    break
                data.extend(chunk)
            data = bytes(data)
        else:
            data = request.get_data()

        metadata = {
            'filename': request.headers.get('X-Filename') or request.args.get('filename'),
            'content_type': request.content_type,
            'content_length': len(data), 'type': data_type
        }
        handler = get_handler(data_type)
        result = handler(data, metadata)
        result['upload_mode'] = 'normal'
        return jsonify(result)

    except Exception as e:
        return jsonify({
            'status': 'error', 'type': data_type,
            'message': str(e)
        }), 500

@app.route('/upload/stream', methods=['POST'])
def upload_stream():
    """ Huge file uploading """
    data_type = request.args.get('type', 'binary')
    temp_file = os.path.join(UPLOAD_FOLDER, f'stream_{hashlib.md5(str(id(request)).encode()).hexdigest()}.tmp')
    try:
        total_size = 0
        md5_hash = hashlib.md5()

        with open(temp_file, 'wb') as f:
            stream = request.stream
            while True:
                chunk = stream.read(CHUNK_SIZE)
                if not chunk:
                    break
                f.write(chunk)
                md5_hash.update(chunk)
                total_size += len(chunk)

        with open(temp_file, 'rb') as f:
            data = f.read()
        metadata = {
            'filename': request.headers.get('X-Filename'),
            'content_type': request.content_type,
            'temp_file': temp_file,
            'streamed': True
        }

        handler = get_handler(data_type)
        result = handler(data, metadata)
        result['upload_mode'] = 'stream'
        result['total_size'] = total_size
        result['md5'] = md5_hash.hexdigest()

        os.remove(temp_file)
        return jsonify(result)

    except Exception as e:
        if os.path.exists(temp_file):
            os.remove(temp_file)
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/status/<upload_id>', methods=['GET'])
def upload_status(upload_id: str):
    """ Check uploading status """
    if upload_id not in chunked_uploads:
        return jsonify({'status': 'not_found', 'upload_id': upload_id}), 404

    session = chunked_uploads[upload_id]
    return jsonify({
        'upload_id': upload_id,
        'received_chunks': len(session['chunks']),
        'total_chunks': session['total'],
        'missing_chunks': [
            i for i in range(session['total'])
            if i not in session['chunks']
        ],
        'type': session['type']
    })

@app.route('/shm/create', methods=['POST'])
def shm_create():
    """ Create SHM """
    if not SHARED_MEMORY_AVAILABLE:
        return jsonify({'status': 'error', 'message': 'SHM not available'}), 503

    data = request.get_json() or {}
    name = data.get('name')
    size = data.get('size', 1024 * 1024)
    if not name:
        return jsonify({'status': 'error', 'message': 'name required'}), 400

    success, msg = shm_mgr.create(name, size)
    if success:
        shm_mgr.set_status(name, LockStatus.SERVER_WRITING)
        return jsonify({
            'status': 'success', 'shm_name': name, 'size': size,
            'total_size': size + ShmHeader.HEADER_SIZE,
            'state': 'SERVER_WRITING',
            'message': 'Server should write data, then set status to READY'
        })
    else:
        return jsonify({'status': 'error', 'message': msg}), 500

@app.route('/shm/status/<name>', methods=['GET'])
def shm_status(name: str):
    """ Check SHM status """
    if not SHARED_MEMORY_AVAILABLE:
        return jsonify({'status': 'error', 'message': 'SHM not available'}), 503

    header = shm_mgr.read_header(name)
    if not header:
        return jsonify({'status': 'error', 'message': 'SHM not found'}), 404
    return jsonify({
        'status': 'success', 'shm_name': name,
        'state': LockStatus(header.status).name,
        'data_size': header.data_size,
        'buffer_size': header.buffer_size,
        'timestamp': header.timestamp
    })

@app.route('/shm/write/<name>', methods=['POST'])
def shm_write(name: str):
    """ Write to SHM """
    if not SHARED_MEMORY_AVAILABLE:
        return jsonify({'status': 'error', 'message': 'SHM not available'}), 503

    data = request.get_data()
    if shm_mgr.write_data(name, data):
        return jsonify({
            'status': 'success', 'bytes_written': len(data),
            'message': 'Use /shm/ready/<name> to mark data as ready'
        })
    else:
        return jsonify({'status': 'error', 'message': 'Failed to write'}), 500

@app.route('/shm/ready/<name>', methods=['POST'])
def shm_ready(name: str):
    """ Notify SHM state """
    if not SHARED_MEMORY_AVAILABLE:
        return jsonify({'status': 'error', 'message': 'SHM not available'}), 503
    if shm_mgr.set_status(name, LockStatus.READY):
        return jsonify({
            'status': 'success',
            'message': 'Client can now read from SHM'
        })
    else:
        return jsonify({'status': 'error', 'message': 'Failed to set status'}), 500

@app.route('/shm/close/<name>', methods=['POST'])
def shm_close(name: str):
    """ Close SHM """
    if shm_mgr.close(name):
        return jsonify({'status': 'success', 'message': 'SHM closed and unlinked'})
    else:
        return jsonify({'status': 'error', 'message': 'Failed to close'}), 500

@app.route('/shm/list', methods=['GET'])
def shm_list():
    """ List all SHMs """
    if not SHARED_MEMORY_AVAILABLE:
        return jsonify({'status': 'error', 'message': 'SHM not available'}), 503
    return jsonify({
        'status': 'success',
        'shared_memories': {
            name: {
                'header': shm_mgr.read_header(name).__dict__ if shm_mgr.read_header(name) else None,
                'metadata': shm_mgr._metadata.get(name, {})
            }
            for name in shm_mgr._shm_map.keys()
        }
    })

@app.errorhandler(413)
def too_large(e):
    return jsonify({
        'status': 'error',
        'message': 'File too large. Use chunked upload mode.',
        'max_size': MAX_CONTENT_LENGTH,
        'solution': 'Add ?mode=chunked and use X-Chunk-* headers'
    }), 413

if __name__ == '__main__':
    """
    Example 1:
        curl -X POST "http://localhost:5000/upload?type=image"
             -H "Content-Type: image/png" --data-binary @image.png
    Example 2:
        curl -X POST "http://localhost:5000/upload?type=file&mode=chunked"
             -H "X-Upload-ID: upload_001" -H "X-Chunk-Index: 0"
             -H "X-Total-Chunks: 3" -H "X-Filename: large_video.mp4" --data-binary @part1
        curl "http://localhost:5000/status/upload_001"
    Example3:
        curl -X POST "http://localhost:5000/upload/stream?type=binary"
             -H "X-Filename: huge_dataset.bin" --data-binary @10gb_file.bin
    """
    print("Starting osgVerse multi-model server...")
    print("Supported types: text, image, binary, json, file, shm")
    print(f"Shared Memory available: {SHARED_MEMORY_AVAILABLE}")
    print("Endpoints:")
    print("  POST /upload?type=<type>                - Normal uploading")
    print("  POST /upload?mode=chunked&type=<type>   - Chunked uploading")
    print("  POST /upload/stream?type=<type>         - Stream uploading for huge files")
    print("  GET  /status/<upload_id>                - Check uploading status")
    print()

    parser = argparse.ArgumentParser(description='Multi-model Server')
    parser.add_argument(
        '-p', '--port', type=int, default=5000,
        help='Port to run the server on (default: 5000)'
    )
    parser.add_argument(
        '--host', type=str, default='0.0.0.0',
        help='Host to bind to (default: 0.0.0.0)'
    )
    args = parser.parse_args()
    app.run(host=args.host, port=args.port, threaded=True)
