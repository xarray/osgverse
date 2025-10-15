// This code was automatically generated; you're not supposed to edit it!

#ifndef GLU_BLELLOCHSCAN_HPP
#define GLU_BLELLOCHSCAN_HPP

#include <string>

#ifndef GLU_REDUCE_HPP
#define GLU_REDUCE_HPP

#ifndef GLU_DATA_TYPES_HPP
#define GLU_DATA_TYPES_HPP

#ifndef GLU_ERRORS_HPP
#define GLU_ERRORS_HPP

#include <cstdio>
#include <cstdlib>

// TODO mark if (!condition_) as unlikely
#define GLU_CHECK_STATE(condition_, ...)                                                                                   \
    {                                                                                                                  \
        if (!(condition_))                                                                                             \
        {                                                                                                              \
            fprintf(stderr, __VA_ARGS__);                                                                              \
            exit(1);                                                                                                   \
        }                                                                                                              \
    }

#define GLU_CHECK_ARGUMENT(condition_, ...) GLU_CHECK_STATE(condition_, __VA_ARGS__)
#define GLU_FAIL(...) GLU_CHECK_STATE(false, __VA_ARGS__)

#endif



namespace glu
{
    enum DataType
    {
        DataType_Float = 0,
        DataType_Double,
        DataType_Int,
        DataType_Uint,
        DataType_Vec2,
        DataType_Vec4,
        DataType_DVec2,
        DataType_DVec4,
        DataType_UVec2,
        DataType_UVec4,
        DataType_IVec2,
        DataType_IVec4
    };

    inline const char* to_glsl_type_str(DataType data_type)
    {
        // clang-format off
        if (data_type == DataType_Float)       return "float";
        else if (data_type == DataType_Double) return "double";
        else if (data_type == DataType_Int)    return "int";
        else if (data_type == DataType_Uint)   return "uint";
        else if (data_type == DataType_Vec2)   return "vec2";
        else if (data_type == DataType_Vec4)   return "vec4";
        else if (data_type == DataType_DVec2)  return "dvec2";
        else if (data_type == DataType_DVec4)  return "dvec4";
        else if (data_type == DataType_UVec2)  return "uvec2";
        else if (data_type == DataType_UVec4)  return "uvec4";
        else if (data_type == DataType_IVec2)  return "ivec2";
        else if (data_type == DataType_IVec4)  return "ivec4";
        else
        {
            GLU_FAIL("Invalid data type: %d", data_type);
        }
        // clang-format on
    }

} // namespace glu

#endif // GLU_DATA_TYPES_HPP


#ifndef GLU_GL_UTILS_HPP
#define GLU_GL_UTILS_HPP

#include <cmath>
#include <functional>
#include <string>
#include <vector>

#ifndef GLU_ERRORS_HPP
#define GLU_ERRORS_HPP

#include <cstdio>
#include <cstdlib>

// TODO mark if (!condition_) as unlikely
#define GLU_CHECK_STATE(condition_, ...)                                                                                   \
    {                                                                                                                  \
        if (!(condition_))                                                                                             \
        {                                                                                                              \
            fprintf(stderr, __VA_ARGS__);                                                                              \
            exit(1);                                                                                                   \
        }                                                                                                              \
    }

#define GLU_CHECK_ARGUMENT(condition_, ...) GLU_CHECK_STATE(condition_, __VA_ARGS__)
#define GLU_FAIL(...) GLU_CHECK_STATE(false, __VA_ARGS__)

#endif



namespace glu
{
    inline void
    copy_buffer(GLuint src_buffer, GLuint dst_buffer, size_t size, size_t src_offset = 0, size_t dst_offset = 0)
    {
        glBindBuffer(GL_COPY_READ_BUFFER, src_buffer);
        glBindBuffer(GL_COPY_WRITE_BUFFER, dst_buffer);

        glCopyBufferSubData(
            GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, (GLintptr) src_offset, (GLintptr) dst_offset, (GLsizeiptr) size
        );
    }

    /// A RAII wrapper for GL shader.
    class Shader
    {
    private:
        GLuint m_handle;

    public:
        explicit Shader(GLenum type) :
            m_handle(glCreateShader(type)){};
        Shader(const Shader&) = delete;

        Shader(Shader&& other) noexcept
        {
            m_handle = other.m_handle;
            other.m_handle = 0;
        }

        ~Shader() { glDeleteShader(m_handle); }

        [[nodiscard]] GLuint handle() const { return m_handle; }

        void source_from_str(const std::string& src_str)
        {
            const char* src_ptr = src_str.c_str();
            glShaderSource(m_handle, 1, &src_ptr, nullptr);
        }

        void source_from_file(const char* src_filepath)
        {
            FILE* file = fopen(src_filepath, "rt");
            GLU_CHECK_STATE(!file, "Failed to shader file: %s", src_filepath);

            fseek(file, 0, SEEK_END);
            size_t file_size = ftell(file);
            fseek(file, 0, SEEK_SET);

            std::string src{};
            src.resize(file_size);
            fread(src.data(), sizeof(char), file_size, file);
            source_from_str(src.c_str());

            fclose(file);
        }

        std::string get_info_log()
        {
            GLint log_length = 0;
            glGetShaderiv(m_handle, GL_INFO_LOG_LENGTH, &log_length);

            std::vector<GLchar> log(log_length);
            glGetShaderInfoLog(m_handle, log_length, nullptr, log.data());
            return {log.begin(), log.end()};
        }

        void compile()
        {
            glCompileShader(m_handle);

            GLint status;
            glGetShaderiv(m_handle, GL_COMPILE_STATUS, &status);
            if (!status)
            {
                GLU_CHECK_STATE(status, "Shader failed to compile: %s", get_info_log().c_str());
            }
        }
    };

    /// A RAII wrapper for GL program.
    class Program
    {
    private:
        GLuint m_handle;

    public:
        explicit Program() { m_handle = glCreateProgram(); };
        Program(const Program&) = delete;

        Program(Program&& other) noexcept
        {
            m_handle = other.m_handle;
            other.m_handle = 0;
        }

        ~Program() { glDeleteProgram(m_handle); }

        [[nodiscard]] GLuint handle() const { return m_handle; }

        void attach_shader(GLuint shader_handle) { glAttachShader(m_handle, shader_handle); }
        void attach_shader(const Shader& shader) { glAttachShader(m_handle, shader.handle()); }

        [[nodiscard]] std::string get_info_log() const
        {
            GLint log_length = 0;
            glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &log_length);

            std::vector<GLchar> log(log_length);
            glGetProgramInfoLog(m_handle, log_length, nullptr, log.data());
            return {log.begin(), log.end()};
        }

        void link()
        {
            GLint status;
            glLinkProgram(m_handle);
            glGetProgramiv(m_handle, GL_LINK_STATUS, &status);
            if (!status)
            {
                GLU_CHECK_STATE(status, "Program failed to link: %s", get_info_log().c_str());
            }
        }

        void use() { glUseProgram(m_handle); }

        GLint get_uniform_location(const char* uniform_name)
        {
            GLint loc = glGetUniformLocation(m_handle, uniform_name);
            GLU_CHECK_STATE(loc >= 0, "Failed to get uniform location: %s", uniform_name);
            return loc;
        }
    };

    /// A RAII helper class for GL shader storage buffer.
    class ShaderStorageBuffer
    {
    private:
        GLuint m_handle = 0;
        size_t m_size = 0;

    public:
        explicit ShaderStorageBuffer(size_t initial_size = 0)
        {
            if (initial_size > 0)
                resize(initial_size, false);
        }

        explicit ShaderStorageBuffer(const void* data, size_t size) :
            m_size(size)
        {
            GLU_CHECK_ARGUMENT(data, "");
            GLU_CHECK_ARGUMENT(size > 0, "");

            glCreateBuffers(1, &m_handle);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
            glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr) m_size, data, GL_DYNAMIC_STORAGE_BIT);
        }

        template<typename T>
        explicit ShaderStorageBuffer(const std::vector<T>& data) :
            ShaderStorageBuffer(data.data(), data.size() * sizeof(T))
        {
        }

        ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
        ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept
        {
            m_handle = other.m_handle;
            m_size = other.m_size;
            other.m_handle = 0;
        }

        ~ShaderStorageBuffer()
        {
            if (m_handle)
                glDeleteBuffers(1, &m_handle);
        }

        [[nodiscard]] GLuint handle() const { return m_handle; }
        [[nodiscard]] size_t size() const { return m_size; }

        /// Grows or shrinks the buffer. If keep_data, performs an additional copy to maintain the data.
        void resize(size_t size, bool keep_data = false)
        {
            size_t old_size = m_size;
            GLuint old_handle = m_handle;

            if (old_size != size)
            {
                m_size = size;

                glCreateBuffers(1, &m_handle);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
                glBufferStorage(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr) m_size, nullptr, GL_DYNAMIC_STORAGE_BIT);

                if (keep_data)
                    copy_buffer(old_handle, m_handle, std::min(old_size, size));

                glDeleteBuffers(1, &old_handle);
            }
        }

        /// Clears the entire buffer with the given GLuint value (repeated).
        void clear(GLuint value)
        {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
            glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &value);
        }

        void write_data(const void* data, size_t size)
        {
            GLU_CHECK_ARGUMENT(size <= m_size, "");

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
        }

        template<typename T>
        std::vector<T> get_data() const
        {
            GLU_CHECK_ARGUMENT(m_size % sizeof(T) == 0, "Size %zu isn't a multiple of %zu", m_size, sizeof(T));

            std::vector<T> result(m_size / sizeof(T));
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr) m_size, result.data());
            return result;
        }

        void bind(GLuint index, size_t size = 0, size_t offset = 0)
        {
            if (size == 0)
                size = m_size;
            glBindBufferRange(GL_SHADER_STORAGE_BUFFER, index, m_handle, (GLintptr) offset, (GLsizeiptr) size);
        }
    };

    /// Measures elapsed time on GPU for executing the given callback.
    inline uint64_t measure_gl_elapsed_time(const std::function<void()>& callback)
    {
        GLuint query;
        uint64_t elapsed_time{};

        glGenQueries(1, &query);
        glBeginQuery(GL_TIME_ELAPSED, query);

        callback();

        glEndQuery(GL_TIME_ELAPSED);

        glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
        glDeleteQueries(1, &query);

        return elapsed_time;
    }

    template<typename IntegerT>
    IntegerT log32_floor(IntegerT n)
    {
        return (IntegerT) floor(double(log2(n)) / 5.0);
    }

    template<typename IntegerT>
    IntegerT log32_ceil(IntegerT n)
    {
        return (IntegerT) ceil(double(log2(n)) / 5.0);
    }

    template<typename IntegerT>
    IntegerT div_ceil(IntegerT n, IntegerT d)
    {
        return (IntegerT) ceil(double(n) / double(d));
    }

    template<typename T>
    bool is_power_of_2(T n)
    {
        return (n & (n - 1)) == 0;
    }

    template<typename IntegerT>
    IntegerT next_power_of_2(IntegerT n)
    {
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n++;
        return n;
    }

    template<typename Iterator>
    void print_stl_container(Iterator begin, Iterator end)
    {
        size_t i = 0;
        for (; begin != end; begin++)
        {
            printf("(%zu) %s, ", i, std::to_string(*begin).c_str());
            i++;
        }
        printf("\n");
    }

    template<typename T>
    void print_buffer(const ShaderStorageBuffer& buffer)
    {
        std::vector<T> data = buffer.get_data<T>();
        print_stl_container(data.begin(), data.end());
    }

    inline void print_buffer_hex(const ShaderStorageBuffer& buffer)
    {
        std::vector<GLuint> data = buffer.get_data<GLuint>();
        for (size_t i = 0; i < data.size(); i++)
            printf("(%zu) %08x, ", i, data[i]);
        printf("\n");
    }
} // namespace glu

#endif // GLU_GL_UTILS_HPP



namespace glu
{
    namespace detail
    {
        inline const char* k_reduction_shader_src = R"(
#extension GL_KHR_shader_subgroup_arithmetic : require

layout(local_size_x = NUM_THREADS, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer Buffer
{
    DATA_TYPE data[];
};

layout(location = 0) uniform uint u_count;
layout(location = 1) uniform uint u_depth;

void main()
{
    uint step = 1 << (5 * u_depth);
    uint subgroup_i = gl_WorkGroupID.x * NUM_THREADS + gl_SubgroupID * gl_SubgroupSize;
    uint i = (subgroup_i + gl_SubgroupInvocationID) * step;
    if (i < u_count)
    {
        DATA_TYPE r = SUBGROUP_OPERATION(data[i]);
        if (gl_SubgroupInvocationID == 0)
        {
            data[i] = r;
        }
    }
}
)";
    }

    /// The operators that can be used for the reduction operation.
    enum ReduceOperator
    {
        ReduceOperator_Sum = 0,
        ReduceOperator_Mul,
        ReduceOperator_Min,
        ReduceOperator_Max
    };

    /// A class that implements the reduction operation.
    class Reduce
    {
    private:
        const DataType m_data_type;
        const ReduceOperator m_operator;
        const size_t m_num_threads;
        const size_t m_num_items;

        Program m_program;

    public:
        explicit Reduce(DataType data_type, ReduceOperator operator_) :
            m_data_type(data_type),
            m_operator(operator_),
            m_num_threads(1024),
            m_num_items(4)
        {
            std::string shader_src = "#version 460\n\n";

            shader_src += std::string("#define DATA_TYPE ") + to_glsl_type_str(m_data_type) + "\n";
            shader_src += std::string("#define NUM_THREADS ") + std::to_string(m_num_threads) + "\n";
            shader_src += std::string("#define NUM_ITEMS ") + std::to_string(m_num_items) + "\n";

            if (m_operator == ReduceOperator_Sum)
            {
                shader_src += "#define OPERATOR(a, b) (a + b)\n";
                shader_src += "#define SUBGROUP_OPERATION(value) subgroupAdd(value)\n";
            }
            else if (m_operator == ReduceOperator_Mul)
            {
                shader_src += "#define OPERATOR(a, b) (a * b)\n";
                shader_src += "#define SUBGROUP_OPERATION(value) subgroupMul(value)\n";
            }
            else if (m_operator == ReduceOperator_Min)
            {
                shader_src += "#define OPERATOR(a, b) (min(a, b))\n";
                shader_src += "#define SUBGROUP_OPERATION(value) subgroupMin(value)\n";
            }
            else if (m_operator == ReduceOperator_Max)
            {
                shader_src += "#define OPERATOR(a, b) (max(a, b))\n";
                shader_src += "#define SUBGROUP_OPERATION(value) subgroupMax(value)\n";
            }
            else
            {
                GLU_FAIL("Invalid reduction operator: %d", m_operator);
            }

            shader_src += detail::k_reduction_shader_src;

            Shader shader(GL_COMPUTE_SHADER);
            shader.source_from_str(shader_src.c_str());
            shader.compile();

            m_program.attach_shader(shader);
            m_program.link();
        }

        ~Reduce() = default;

        void operator()(GLuint buffer, size_t count)
        {
            GLU_CHECK_ARGUMENT(buffer, "Invalid buffer");
            GLU_CHECK_ARGUMENT(count > 0, "Count must be greater than zero");

            m_program.use();

            glUniform1ui(m_program.get_uniform_location("u_count"), count);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);

            for (int depth = 0;; depth++)
            {
                int step = 1 << (5 * depth);
                if (step >= count)
                    break;

                size_t level_count = count >> (5 * depth);

                glUniform1ui(m_program.get_uniform_location("u_depth"), depth);

                size_t num_workgroups = div_ceil(level_count, m_num_threads);
                glDispatchCompute(num_workgroups, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }
        }
    };
} // namespace glu

#endif // GLU_REDUCE_HPP


#ifndef GLU_DATA_TYPES_HPP
#define GLU_DATA_TYPES_HPP

#ifndef GLU_ERRORS_HPP
#define GLU_ERRORS_HPP

#include <cstdio>
#include <cstdlib>

// TODO mark if (!condition_) as unlikely
#define GLU_CHECK_STATE(condition_, ...)                                                                                   \
    {                                                                                                                  \
        if (!(condition_))                                                                                             \
        {                                                                                                              \
            fprintf(stderr, __VA_ARGS__);                                                                              \
            exit(1);                                                                                                   \
        }                                                                                                              \
    }

#define GLU_CHECK_ARGUMENT(condition_, ...) GLU_CHECK_STATE(condition_, __VA_ARGS__)
#define GLU_FAIL(...) GLU_CHECK_STATE(false, __VA_ARGS__)

#endif



namespace glu
{
    enum DataType
    {
        DataType_Float = 0,
        DataType_Double,
        DataType_Int,
        DataType_Uint,
        DataType_Vec2,
        DataType_Vec4,
        DataType_DVec2,
        DataType_DVec4,
        DataType_UVec2,
        DataType_UVec4,
        DataType_IVec2,
        DataType_IVec4
    };

    inline const char* to_glsl_type_str(DataType data_type)
    {
        // clang-format off
        if (data_type == DataType_Float)       return "float";
        else if (data_type == DataType_Double) return "double";
        else if (data_type == DataType_Int)    return "int";
        else if (data_type == DataType_Uint)   return "uint";
        else if (data_type == DataType_Vec2)   return "vec2";
        else if (data_type == DataType_Vec4)   return "vec4";
        else if (data_type == DataType_DVec2)  return "dvec2";
        else if (data_type == DataType_DVec4)  return "dvec4";
        else if (data_type == DataType_UVec2)  return "uvec2";
        else if (data_type == DataType_UVec4)  return "uvec4";
        else if (data_type == DataType_IVec2)  return "ivec2";
        else if (data_type == DataType_IVec4)  return "ivec4";
        else
        {
            GLU_FAIL("Invalid data type: %d", data_type);
        }
        // clang-format on
    }

} // namespace glu

#endif // GLU_DATA_TYPES_HPP



namespace glu
{
    namespace detail
    {
        inline const char* k_upsweep_shader_src = R"(
#extension GL_KHR_shader_subgroup_shuffle_relative : require

layout(local_size_x = NUM_THREADS, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer Buffer
{
    DATA_TYPE data[];
};

layout(location = 0) uniform uint u_count;
layout(location = 1) uniform uint u_step;

void main()
{
    uint partition_i = gl_WorkGroupID.y;
    uint thread_i = gl_WorkGroupID.x * NUM_THREADS + gl_SubgroupID * gl_SubgroupSize + gl_SubgroupInvocationID;
    uint i = partition_i * u_count + thread_i * u_step + u_step - 1;
    uint end_i = (partition_i + 1) * u_count;
    if (i < end_i)
    {
        DATA_TYPE lval = subgroupShuffleUp(data[i], 1);
        DATA_TYPE r = OPERATION(data[i], lval);
        if (i == end_i - 1)  // Clear last
        {
            data[i] = IDENTITY;
        }
        else if (gl_SubgroupInvocationID % 2 == 1)
        {
            data[i] = r;
        }
    }
}
)";

        inline const char* k_downsweep_shader_src = R"(
layout(local_size_x = NUM_THREADS, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer Buffer
{
    DATA_TYPE data[];
};

layout(location = 0) uniform uint u_count;
layout(location = 1) uniform uint u_step;

void main()
{
    uint partition_i = gl_WorkGroupID.y;
    uint i = partition_i * u_count + gl_GlobalInvocationID.x * (u_step << 1) + (u_step - 1);
    uint next_i = i + u_step;
    uint end_i = (partition_i + 1) * u_count;
    if (next_i < end_i)
    {
        DATA_TYPE tmp = data[i];
        data[i] = data[next_i];
        data[next_i] = data[next_i] + tmp;
    }
    else if (i < end_i)
    {
        data[i] = IDENTITY;
    }
}
)";
    } // namespace detail

    /// A class that implements Blelloch scan algorithm (exclusive prefix sum).
    class BlellochScan
    {
    private:
        const DataType m_data_type;
        const size_t m_num_threads;
        const size_t m_num_items;

        Program m_upsweep_program;
        Program m_downsweep_program;

    public:
        explicit BlellochScan(DataType data_type) :
            m_data_type(data_type),
            m_num_threads(1024),
            m_num_items(4)
        {
            std::string shader_src = "#version 460\n\n";

            shader_src += std::string("#define DATA_TYPE ") + to_glsl_type_str(m_data_type) + "\n";
            shader_src += "#define OPERATION(a, b) (a + b)\n";
            shader_src += "#define IDENTITY 0\n";
            shader_src += std::string("#define NUM_THREADS ") + std::to_string(m_num_threads) + "\n";
            shader_src += std::string("#define NUM_ITEMS ") + std::to_string(m_num_items) + "\n";

            { // Upsweep program
                Shader upsweep_shader(GL_COMPUTE_SHADER);
                upsweep_shader.source_from_str((shader_src + detail::k_upsweep_shader_src).c_str());
                upsweep_shader.compile();

                m_upsweep_program.attach_shader(upsweep_shader);
                m_upsweep_program.link();
            }

            { // Downsweep program
                Shader downsweep_program(GL_COMPUTE_SHADER);
                downsweep_program.source_from_str((shader_src + detail::k_downsweep_shader_src).c_str());
                downsweep_program.compile();

                m_downsweep_program.attach_shader(downsweep_program);
                m_downsweep_program.link();
            }
        }

        ~BlellochScan() = default;

        /// Runs Blelloch exclusive scan on multiple partitions.
        ///
        /// @param buffer the input GLuint buffer
        /// @param count the number of GLuint in the buffer (must be a power of 2)
        /// @param num_partitions the number of partitions (must be adjacent)
        void operator()(GLuint buffer, size_t count, size_t num_partitions = 1)
        {
            GLU_CHECK_ARGUMENT(buffer, "Invalid buffer");
            GLU_CHECK_ARGUMENT(count > 0, "Count must be greater than zero");
            GLU_CHECK_ARGUMENT(is_power_of_2(count), "Count must be a power of 2"); // TODO Remove this requirement
            GLU_CHECK_ARGUMENT(num_partitions >= 1, "Num of partitions must be >= 1");

            upsweep(buffer, count, num_partitions); // Also clear last
            downsweep(buffer, count, num_partitions);
        }

    private:
        void upsweep(GLuint buffer, size_t count, size_t num_partitions) // Also clear last
        {
            m_upsweep_program.use();

            glUniform1ui(m_upsweep_program.get_uniform_location("u_count"), count);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);

            int step = 1;
            int level_count = (int) count;
            while (true)
            {
                glUniform1ui(m_upsweep_program.get_uniform_location("u_step"), step);

                size_t num_workgroups = div_ceil<size_t>(level_count, m_num_threads);
                glDispatchCompute(num_workgroups, num_partitions, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                step <<= 1;

                level_count >>= 1;

                if (level_count <= 1)
                    break;
            }
        }

        void downsweep(GLuint buffer, size_t count, size_t num_partitions)
        {
            m_downsweep_program.use();

            glUniform1ui(m_downsweep_program.get_uniform_location("u_count"), count);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);

            int step = next_power_of_2(int(count)) >> 1;
            size_t level_count = 1;
            while (true)
            {
                glUniform1ui(m_downsweep_program.get_uniform_location("u_step"), step);

                size_t num_workgroups = div_ceil(level_count, m_num_threads);
                glDispatchCompute(num_workgroups, num_partitions, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                step >>= 1;
                level_count <<= 1;
                if (step == 0)
                    break;
            }
        }
    };
} // namespace glu

#endif // GLU_BLELLOCHSCAN_HPP
