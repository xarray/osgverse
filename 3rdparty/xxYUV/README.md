# xxYUV
Convert between RGB and YUV

## Benchmark Environment
https://github.com/metarutaiga/xxImGui/tree/experimental

## Performance (macOS) Encode / Decode (unit : us)
                                                    
|            |           | Encode      |             | Decode      |             |
| ---------- | --------- | ----------- | ----------- | ----------- | ----------- |
|            | Apple M1  | YU12 / YV12 | NV12 / NV21 | YU12 / YV12 | NV12 / NV21 |
| xxYUV      | AMX       |           ? |           ? |          67 |           ? |
| xxYUV      | NEON      |          37 |          38 |          38 |          42 |
| libyuv     | NEON      |          48 |          49 |         122 |          89 |
| Accelerate | NEON      |          56 |          55 |          62 |          59 |
| xxYUV      | SSSE3     |         134 |         133 |             |             |
| libyuv     | SSSE3     |         146 |         146 |         171 |         164 |
| Accelerate | SSSE3     |         273 |         274 |         232 |         231 |
| xxYUV      | SSE2      |         142 |         143 |          58 |          56 |

|            |           | Encode      |             | Decode      |             |
| ---------- | --------- | ----------- | ----------- | ----------- | ----------- |
|            | i7-8700B  | YU12 / YV12 | NV12 / NV21 | YU12 / YV12 | NV12 / NV21 |
| xxYUV      | AVX2      |          31 |          33 |          46 |          39 |
| libyuv     | AVX2      |          48 |          39 |          60 |          54 |
| Accelerate | AVX2      |          83 |          84 |          67 |          62 |
| xxYUV      | SSSE3     |          50 |          51 |             |             |
| libyuv     | SSSE3     |          60 |          61 |          87 |          82 |
| xxYUV      | SSE2      |          90 |          91 |          69 |          62 |
