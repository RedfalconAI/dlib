// Copyright (C) 2015  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_TeNSOR_TOOLS_H_
#define DLIB_TeNSOR_TOOLS_H_

#include "tensor.h"
#include "cudnn_dlibapi.h"
#include "cublas_dlibapi.h"
#include "curand_dlibapi.h"
#include "cpu_dlib.h"
#include "cuda_dlib.h"
#include "../rand.h"

namespace dlib { namespace tt
{

// ----------------------------------------------------------------------------------------

    void gemm (
        float beta,
        tensor& dest,
        float alpha,
        const tensor& lhs,
        bool trans_lhs,
        const tensor& rhs,
        bool trans_rhs
    );
    /*!
        requires
            - The dimensions of lhs and rhs must be compatible for matrix multiplication.
              In particular:
                - Let L == trans_lhs ? trans(mat(lhs)) : mat(lhs)
                - Let R == trans_rhs ? trans(mat(rhs)) : mat(rhs)
                - Let D == mat(dest)
                - D.nr() == L.nr() && D.nc() == R.nc()
                  (i.e. dest must be preallocated and have the correct output dimensions)
                - L.nc() == R.nr()
        ensures
            - performs: dest = alpha*L*R + beta*mat(dest)
    !*/

// ----------------------------------------------------------------------------------------

    class tensor_rand
    {
        /*!
            WHAT THIS OBJECT REPRESENTS
                This is a tool for filling a tensor with random numbers.  

                Note that the sequence of random numbers output by this object is different
                when dlib is compiled with DLIB_USE_CUDA.  So you should not write code
                that depends on any specific sequence of numbers coming out of a
                tensor_rand.

        !*/

    public:
        // not copyable
        tensor_rand(const tensor_rand&) = delete;
        tensor_rand& operator=(const tensor_rand&) = delete;

        tensor_rand() : tensor_rand(0) {}
        tensor_rand(unsigned long long seed);

        void fill_gaussian (
            tensor& data,
            float mean,
            float stddev
        );
        /*!
            requires
                - data.size()%2 == 0
            ensures
                - Fills data with random numbers drawn from a Gaussian distribution
                  with the given mean and standard deviation.
        !*/

        void fill_uniform (
            tensor& data
        );
        /*!
            ensures
                - Fills data with uniform random numbers in the range (0.0, 1.0].
        !*/

#ifdef DLIB_USE_CUDA
        cuda::curand_generator rnd;
#else
        dlib::rand rnd;
#endif
    };

// ----------------------------------------------------------------------------------------

    void multiply (
        tensor& dest,
        const tensor& src1,
        const tensor& src2
    );
    /*!
        requires
            - dest.k()  == src1.k()  == src2.k()
            - dest.nr() == src1.nr() == src2.nr()
            - dest.nc() == src1.nc() == src2.nc()
            - dest.num_samples(), src1.num_samples(), and src2.num_samples() must each
              either be 1 or whichever ones aren't equal to 1 must have the same values.
        ensures
            - let MD = max(dest.num_samples(), src1.num_samples(), src2.num_samples)
            - This function pointwise multiplies src1 with src2 and stores the result into
              #dest.  However, how the multiplication happens depends on the dimensions of
              the tensors.  First, when src1 and src2 are multiplied together, if either
              has a num_samples() dimension that is != MD, then it is first replicated to
              produce a tensor with num_samples()==MD dimensions and then they are
              pointwise multiplied together.

              Second, if dest.num_samples()==1, then after the pointwise multiplication of
              src1 with src2, the result has its samples summed to produce an output tensor
              with num_samples()==1 which is then assigned to #dest.
    !*/

// ----------------------------------------------------------------------------------------

    void affine_transform(
        tensor& dest,
        const tensor& src,
        const float A,
        const float B
    );
    /*!
        requires
            - dest.size()==src.size()
        ensures
            - #dest == A*src + B
    !*/

    void affine_transform(
        tensor& dest,
        const tensor& src1,
        const tensor& src2,
        const float A,
        const float B,
        const float C
    );
    /*!
        requires
            - dest.size()==src1.size()
            - dest.size()==src2.size()
        ensures
            - #dest == A*src1 + src2*B + C
    !*/

    void affine_transform(
        tensor& dest,
        const tensor& src1,
        const tensor& src2,
        const tensor& src3,
        const float A,
        const float B,
        const float C,
        const float D
    );
    /*!
        requires
            - dest.size()==src1.size()
            - dest.size()==src2.size()
            - dest.size()==src3.size()
        ensures
            - #dest == A*src1 + src2*B + src3*C + D
    !*/

// ----------------------------------------------------------------------------------------

    void affine_transform(
        tensor& dest,
        const tensor& src,
        const tensor& A,
        const tensor& B
    );
    /*!
        requires
            - have_same_dimensions(dest,src) == true
            - if (A.num_samples() == 1) then
                - B.num_samples() == 1
            - else
                - A.num_samples() == src.num_samples()
                - B.num_samples() == src.num_samples()
            - A.nr() == B.nr() == src.nr()
            - A.nc() == B.nc() == src.nc()
            - A.k()  == B.k()  == src.k()
        ensures
            - if (A.num_samples() == 1) then
                - #dest == A*src + B
                    (done for each sample in src)
            - else
                - for all valid i:
                    - #dest.host()[i] == A.host()[i]*src.host()[i] + B.host()[i]  
    !*/

// ----------------------------------------------------------------------------------------

    void batch_normalize (
        resizable_tensor& dest,
        resizable_tensor& means,
        resizable_tensor& invstds,
        const tensor& src,
        const tensor& gamma, 
        const tensor& beta 
    );
    /*!
        requires
            - src.num_samples() > 1
            - gamma.num_samples() == 1
            - beta.num_samples() == 1
            - gamma.nr() == beta.nr() == src.nr()
            - gamma.nc() == beta.nc() == src.nc()
            - gamma.k()  == beta.k()  == src.k()
        ensures
            - have_same_dimensions(#dest, src) == true
            - #means.num_samples() == 1
            - #invstds.num_samples() == 1
            - means.nr() == invstds.nr() == src.nr()
            - means.nc() == invstds.nc() == src.nc()
            - means.k()  == invstds.k()  == src.k()
            - #src == the batch normalized version of src.
            - #means == the mean values of the contents of src.
            - #invstds == 1/(the standard deviation values of the contents of src).
    !*/

    class batch_normalize_gradient
    {
    public:
        void operator() (
            const tensor& gradient_input,
            const tensor& means,
            const tensor& invstds,
            const tensor& src,
            const tensor& gamma,
            tensor& src_grad,
            tensor& gamma_grad, 
            tensor& beta_grad 
        ){impl(gradient_input,means,invstds,src,gamma,src_grad,gamma_grad,beta_grad);}
        /*!
            requires
                - invstds and means should be the output of a call to
                  batch_normalize(dest,means,invstds,src,gamma,beta)
                - have_same_dimensions(gradient_input, src) == true
                - have_same_dimensions(src, src_grad) == true
                - src.num_samples() > 1
                - gamma.num_samples() == 1
                - have_same_dimensions(gamma, gamma_grad) == true
                - have_same_dimensions(gamma, beta_grad) == true
                - gamma.nr() == src.nr()
                - gamma.nc() == src.nc()
                - gamma.k()  == src.k()
                - have_same_dimensions(means, gamma) == true
                - have_same_dimensions(invstds, gamma) == true
            ensures
                - Let f(src,gamma,beta) == dot(gradient_input, dest output of
                  batch_normalize(dest,means,invstds,src,gamma,beta))
                - Adds the gradient of f() with respect to src to #src_grad.
                - Assigns the gradient of f() with respect to gamma to #gamma_grad.
                - Assigns the gradient of f() with respect to beta to #beta_grad.
        !*/
    private:
#ifdef DLIB_USE_CUDA
        cuda::batch_normalize_gradient impl;
#else
        cpu::batch_normalize_gradient impl;
#endif
    };

// ----------------------------------------------------------------------------------------

    void batch_normalize_conv (
        resizable_tensor& dest,
        resizable_tensor& means,
        resizable_tensor& invstds,
        const tensor& src,
        const tensor& gamma, 
        const tensor& beta 
    );
    /*!
        requires
            - src.num_samples() > 1
            - gamma.num_samples()==gamma.nr()==gamma.nc() == 1
            - beta.num_samples() ==beta.nr() ==gamma.nc() == 1
            - gamma.k()  == beta.k()  == src.k()
        ensures
            - have_same_dimensions(#dest, src) == true
            - #means.num_samples()==means.nr()==means.nc() == 1
            - #invstds.num_samples() ==invstds.nr() ==invstds.nc() == 1
            - means.k()  == invstds.k()  == src.k()
            - #src == the batch normalized version of src.
            - #means == the mean values of the contents of src.
            - #invstds == 1/(the standard deviation values of the contents of src).
    !*/

    class batch_normalize_conv_gradient
    {
    public:
        void operator() (
            const tensor& gradient_input,
            const tensor& means,
            const tensor& invstds,
            const tensor& src,
            const tensor& gamma,
            tensor& src_grad,
            tensor& gamma_grad, 
            tensor& beta_grad 
        ){impl(gradient_input,means,invstds,src,gamma,src_grad,gamma_grad,beta_grad);}
        /*!
            requires
                - invstds and means should be the output of a call to
                  batch_normalize_conv(dest,means,invstds,src,gamma,beta)
                - have_same_dimensions(gradient_input, src) == true
                - have_same_dimensions(src, src_grad) == true
                - src.num_samples() > 1
                - gamma.num_samples()==gamma.nr()==gamma.nc() == 1
                - have_same_dimensions(gamma, gamma_grad) == true
                - have_same_dimensions(gamma, beta_grad) == true
                - gamma.k()  == src.k()
                - have_same_dimensions(means, gamma) == true
                - have_same_dimensions(invstds, gamma) == true
            ensures
                - Let f(src,gamma,beta) == dot(gradient_input, dest output of
                  batch_normalize_conv(dest,means,invstds,src,gamma,beta))
                - Adds the gradient of f() with respect to src to #src_grad.
                - Assigns the gradient of f() with respect to gamma to #gamma_grad.
                - Assigns the gradient of f() with respect to beta to #beta_grad.
        !*/
    private:
#ifdef DLIB_USE_CUDA
        cuda::batch_normalize_conv_gradient impl;
#else
        cpu::batch_normalize_conv_gradient impl;
#endif
    };

// -----------------------------------------------------------------------------------

    void threshold (
        tensor& data,
        float thresh
    );
    /*!
        ensures
            - Sets all elements of data to 1 or 0 depending on if they are above or below
              the given threshold.  Specifically, for all valid i:
                - #data.host()[i] == data.host()[i]>thresh ? 1 : 0
    !*/

// ----------------------------------------------------------------------------------------

    void add(
        float beta,
        tensor& dest,
        float alpha,
        const tensor& src
    );
    /*!
        requires
            - One of the following is true: 
                - have_same_dimensions(src, dest)
                - src.num_samples()==1 && src.k()==dest.k() && src.nr()==1 && src.nc()==1
                - src.num_samples()==1 && src.k()==dest.k() && src.nr()==dest.nr() && src.nc()==dest.nc()
                - src.num_samples()==1 && src.k()==1 && src.nr()==dest.nr() && src.nc()==dest.nc()
            - is_same_object(src,dest) == false
        ensures
            - performs: dest = beta*dest + alpha*src
              However, how the addition happens depends on the dimensions of src.  In
              particular, this function adds the scaled values of one src tensor to dest.
              Each dimension of the src tensor must match the corresponding dimension of
              the dest tensor or must be equal to 1. In the latter case, the same value
              from the src tensor, for those dimensions, will be used to add into the dest
              tensor.
    !*/

// ----------------------------------------------------------------------------------------

    void add_conv_bias_gradient (
        tensor& grad,
        const tensor& gradient_input
    );
    /*!
        requires
            - grad.num_samples() == 1
            - grad.k()  >= 1
            - grad.nr() == 1
            - grad.nc() == 1
            - gradient_input.k() == grad.k()
            - gradient_input.size() > 0
            - is_same_object(grad,gradient_input) == false
        ensures
            - let BIAS be a tensor with the same dimensions as grad.
            - let OUT be the output of add(1,OUT,1,BIAS)
            - let f(gradient_input,BIAS) == dot(gradient_input,OUT)
            - Then this function computes the gradient of f() with respect to BIAS and
              assigns it to grad.
    !*/

// ----------------------------------------------------------------------------------------

    void add_bias_gradient (
        tensor& grad,
        const tensor& gradient_input
    );
    /*!
        requires
            - grad.num_samples() == 1
            - gradient_input.k() == grad.k()
            - gradient_input.nr() == grad.nr()
            - gradient_input.nc() == grad.nc()
            - gradient_input.size() > 0
            - is_same_object(grad,gradient_input) == false
        ensures
            - let BIAS be a tensor with the same dimensions as grad.
            - let OUT be the output of add(1,OUT,1,BIAS)
            - let f(gradient_input,BIAS) == dot(gradient_input,OUT)
            - Then this function computes the gradient of f() with respect to BIAS and
              assigns it to grad.
    !*/

// ----------------------------------------------------------------------------------------

    class tensor_conv
    {
    public:
        tensor_conv(const tensor_conv&) = delete;
        tensor_conv& operator=(const tensor_conv&) = delete;

        tensor_conv();

        void clear(
        );

        void operator() (
            resizable_tensor& output,
            const tensor& data,
            const tensor& filters,
            int stride_y,
            int stride_x
        );
        /*!
            requires
                - stride_y > 0
                - stride_x > 0
                - is_same_object(output,data) == false
                - is_same_object(output,filters) == false
            ensures
                - convolves filters over data.  
                    - filters contains filters.num_samples() filters. 
                    - #output.num_samples() == data.num_samples()
                    - #output.k() == filters.num_samples()
                    - #output.nr() == 1+(data.nr()-filters.nr()%2)/stride_y
                    - #output.nc() == 1+(data.nc()-filters.nc()%2)/stride_x
        !*/

        void get_gradient_for_data (
            const tensor& gradient_input, 
            const tensor& filters,
            tensor& data_gradient
        );
        /*!
            requires
                - filters has the same dimensions as the filters object give to the last
                  call to operator().
                - data_gradient has the same dimensions as the data object give to the last
                  call to operator().
                - gradient_input has the same dimensions as the output of operator().
                - is_same_object(data_gradient,filters) == false
                - is_same_object(data_gradient,gradient_input) == false
            ensures
                - let OUT be the output of (*this)(OUT,data,filters).
                - let f(data,filters) == dot(OUT, gradient_input)
                - This function finds the gradient of f() with respect to data and adds
                  this gradient to data_gradient.
        !*/

        void get_gradient_for_filters (
            const tensor& gradient_input, 
            const tensor& data,
            tensor& filters_gradient
        );
        /*!
            requires
                - filters_gradient has the same dimensions as the filters object give to
                  the last call to operator().
                - data has the same dimensions as the data object give to the last call to
                  operator().
                - gradient_input has the same dimensions as the output of operator().
                - is_same_object(filters_gradient,data) == false
                - is_same_object(filters_gradient,gradient_input) == false
            ensures
                - let OUT be the output of (*this)(OUT,data,filters).
                - let f(data,filters) == dot(OUT, gradient_input)
                - This function finds the gradient of f() with respect to filters and assigns 
                  this gradient to filters_gradient.
        !*/

    private:
#ifdef DLIB_USE_CUDA
        cuda::tensor_conv impl;
#else
        // TODO
#endif

    };

// ----------------------------------------------------------------------------------------

    class max_pool
    {
        /*!
        !*/
    public:

        max_pool(const max_pool&) = delete;
        max_pool& operator=(const max_pool&) = delete;

        max_pool (
        );

        void clear(
        );

        void setup(
            int window_height,
            int window_width,
            int stride_y,
            int stride_x
        );

        void operator() (
            resizable_tensor& dest,
            const tensor& src
        );
        /*!
            requires
                - is_same_object(dest,src) == false
            ensures
                - #dest.num_samples() == src.num_samples()
                - #dest.k() == src.k()
                - #dest.nr() == src.nr()/stride_y
                - #dest.nc() == src.nc()/stride_x
                - for all valid s, k, r, and c:
                    - image_plane(#dest,s,k)(r,c) == max(subm_clipped(image_plane(src,s,k),
                                                                        r*stride_y,
                                                                        c*stride_x,
                                                                        window_height,
                                                                        window_width))
        !*/

        void get_gradient(
            const tensor& gradient_input, 
            const tensor& dest,
            const tensor& src,
            tensor& grad 
        );
        /*!
            requires
                - have_same_dimensions(gradient_input,dest) == true
                - have_same_dimensions(src,grad) == true
                - dest contains the result of calling (*this)(dest,src)
                - is_same_object(grad,gradient_input) == false
                - is_same_object(grad,dest) == false
                - is_same_object(grad,src) == false
            ensures
                - Recalling that dest is the output of (*this)(dest,src),
                  let f(src) == dot(gradient_input,dest)
                - Then this function computes the gradient of f() with respect to src and
                  adds it to grad.
        !*/

        private:
#ifdef DLIB_USE_CUDA
        cuda::max_pool impl;
#else
        // TODO
#endif
    };

// ----------------------------------------------------------------------------------------

    void softmax (
        tensor& dest,
        const tensor& src
    );
    /*!
        requires
            - have_same_dimensions(dest, src) == true
        ensures
            - Note that the softmax function is a vector valued function: 
                s(x) == exp(x)/sum(exp(x)) 
            - Computes the softmax function on src and writes the results to dest.  The
              softmax is computed per spatial location across the different channels at
              each location.  That is, softmax() outputs a new tensor, #dest, where each of
              the spatial locations in dest (i.e. image idx, row idx, and column idx)
              contains the output of s() evaluated over the channel values at each
              location.
            - This function supports in-place operation, i.e. having
              is_same_object(dest, src)==true
    !*/

    void softmax_gradient (
        tensor& grad,
        const tensor& dest,
        const tensor& gradient_input
    );
    /*!
        requires
            - have_same_dimensions(dest,gradient_input) == true 
            - have_same_dimensions(dest,grad) == true 
            - is_same_object(grad, dest)==false
        ensures
            - We interpret dest as the output of softmax(dest,SRC) for some SRC tensor.
              Then let f(SRC) == dot(gradient_input,dest) Then this function computes the
              gradient of f() with respect to SRC and adds it to grad.
            - This function supports in-place operation, i.e. having
              is_same_object(grad, gradient_input)==true
    !*/

// ----------------------------------------------------------------------------------------

    void sigmoid (
        tensor& dest,
        const tensor& src
    );
    /*!
        requires
            - have_same_dimensions(dest, src) == true
        ensures
            - for all valid i:
                - #dest.host()[i] == 1/(1+std::exp(-src.host()[i])) 
            - This function supports in-place operation, i.e. having
              is_same_object(dest, src)==true
    !*/

    void sigmoid_gradient (
        tensor& grad,
        const tensor& dest,
        const tensor& gradient_input
    );
    /*!
        requires
            - have_same_dimensions(dest,gradient_input) == true 
            - have_same_dimensions(dest,grad) == true 
            - is_same_object(grad,dest) == false
        ensures
            - Recalling that dest is the output of sigmoid(dest,SRC) for some SRC tensor,
              let f(SRC) == dot(gradient_input,dest)
            - Then this function computes the gradient of f() with respect to SRC and
              assigns it to grad.
            - This function supports in-place operation, i.e. having
              is_same_object(grad, gradient_input)==true
    !*/

// ----------------------------------------------------------------------------------------

    void relu (
        tensor& dest,
        const tensor& src
    );
    /*!
        requires
            - have_same_dimensions(dest, src) == true
        ensures
            - for all valid i:
                - #dest.host()[i] == std::max(0,src.host()[i]) 
            - This function supports in-place operation, i.e. having
              is_same_object(dest, src)==true
    !*/

    void relu_gradient (
        tensor& grad,
        const tensor& dest,
        const tensor& gradient_input
    );
    /*!
        requires
            - have_same_dimensions(dest,gradient_input) == true 
            - have_same_dimensions(dest,grad) == true 
            - is_same_object(grad,dest) == false
        ensures
            - Recalling that dest is the output of relu(dest,SRC) for some SRC tensor,
              let f(SRC) == dot(gradient_input,dest)
            - Then this function computes the gradient of f() with respect to SRC and
              assigns it to grad.
            - This function supports in-place operation, i.e. having
              is_same_object(grad, gradient_input)==true
    !*/

// ----------------------------------------------------------------------------------------

    void tanh (
        tensor& dest,
        const tensor& src
    );
    /*!
        requires
            - have_same_dimensions(dest, src) == true
        ensures
            - for all valid i:
                - #dest.host()[i] == std::tanh(src.host()[i]) 
            - This function supports in-place operation, i.e. having
              is_same_object(dest, src)==true
    !*/

    void tanh_gradient (
        tensor& grad,
        const tensor& dest,
        const tensor& gradient_input
    );
    /*!
        requires
            - have_same_dimensions(dest,gradient_input) == true 
            - have_same_dimensions(dest,grad) == true 
            - is_same_object(grad,dest) == false
        ensures
            - Recalling that dest is the output of tanh(dest,SRC) for some SRC tensor,
              let f(SRC) == dot(gradient_input,dest)
            - Then this function computes the gradient of f() with respect to SRC and
              assigns it to grad.
            - This function supports in-place operation, i.e. having
              is_same_object(grad, gradient_input)==true
    !*/

// ----------------------------------------------------------------------------------------

}}

#ifdef NO_MAKEFILE
#include "tensor_tools.cpp"
#endif

#endif // DLIB_TeNSOR_TOOLS_H_


