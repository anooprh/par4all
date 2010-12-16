#ifndef P4A_ACCEL_WRAPPER_CUDA_H
#define P4A_ACCEL_WRAPPER_CUDA_H

/** A declaration attribute of a hardware-accelerated kernel in CUDA
    called from the GPU it-self
*/
#define P4A_accel_kernel __device__ void

/** A declaration attribute of a hardware-accelerated kernel called from
    the host in CUDA */
#define P4A_accel_kernel_wrapper __global__ void

/** The address space visible for all functions. 
    Allocation in the global memory pool.
 */
#define P4A_accel_global_address

/** The address space in the global memory pool but in read-only mode.
 */
#define P4A_accel_constant_address __constant__

/** The <<shared>> memory in the CUDA architecture.
 */
#define P4A_accel_local_address __shared__

/** Get the coordinate of the virtual processor in X (first) dimension in
    CUDA
*/
#define P4A_vp_0 (blockIdx.x*blockDim.x + threadIdx.x)

/** Get the coordinate of the virtual processor in Y (second) dimension in
    CUDA
*/
#define P4A_vp_1 (blockIdx.y*blockDim.y + threadIdx.y)

/** Get the coordinate of the virtual processor in Z (second) dimension in
    CUDA
*/
#define P4A_vp_2 (blockIdx.z*blockDim.z + threadIdx.z)


#endif //P4A_ACCEL_WRAPPER_CUDA_H
