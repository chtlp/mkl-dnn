/*******************************************************************************
* Copyright 2016 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef CONVOLUTION_FWD_PD_HPP
#define CONVOLUTION_FWD_PD_HPP

#include "mkldnn.h"

#include "c_types_map.hpp"
#include "primitive_desc.hpp"
#include "memory_pd.hpp"
#include "utils.hpp"

namespace mkldnn {
namespace impl {

template <bool with_relu>
struct _convolution_fwd_pd_t: public primitive_desc_t {
    typedef _convolution_fwd_pd_t base_class;
    typedef typename utils::conditional<with_relu,
            convolution_relu_desc_t, convolution_desc_t>::type base_desc_t;
    static constexpr auto base_pkind =
        utils::conditional_v<with_relu, primitive_kind_t,
        primitive_kind::convolution_relu, primitive_kind::convolution>::value;

    _convolution_fwd_pd_t(mkldnn::impl::engine_t *engine,
            const base_desc_t *adesc, const _convolution_fwd_pd_t *hint_fwd_pd)
        : primitive_desc_t(engine, base_pkind), desc_(*adesc)
        , hint_fwd_pd_(hint_fwd_pd) {}
    virtual ~_convolution_fwd_pd_t() {}

    const base_desc_t *desc() const { return &desc_; }
    inline const convolution_desc_t *cdesc() const { return &cdesc_(); }
    virtual const op_desc_t *op_desc() const override
    { return reinterpret_cast<const op_desc_t *>(this->desc()); }

    virtual const memory_pd_t *input_pd(int index = 0) const override {
        switch (index) {
        case 0: return src_pd();
        case 1: case 2: return weights_pd(index - 1);
        default: return nullptr;
        }
    }
    virtual const memory_pd_t *output_pd(int index = 0) const override
    { return index == 0 ? dst_pd() : nullptr; }

    virtual int n_inputs() const override { return 2 + with_bias(); }
    virtual int n_outputs() const override { return 1; }

    virtual status_t query(query_t what, int idx, void *result) const override
    {
        switch (what) {
        case pkind_trait<base_pkind>::query_d:
            *(const base_desc_t**)result = desc(); break;
        default: return primitive_desc_t::query(what, idx, result);
        }
        return status::success;
    }

    /* common conv aux functions */

    inline int MB() const { return cdesc_().src_desc.dims[0]; }

    inline int IC() const { return cdesc_().src_desc.dims[1]; }
    inline int OC() const { return cdesc_().dst_desc.dims[1]; }
    inline int G() const
    { return with_groups() ? cdesc_().weights_desc.dims[0] : 1; }

    inline int IH() const { return cdesc_().src_desc.dims[2]; }
    inline int IW() const { return cdesc_().src_desc.dims[3]; }
    inline int OH() const { return cdesc_().dst_desc.dims[2]; }
    inline int OW() const { return cdesc_().dst_desc.dims[3]; }
    inline int KH() const
    { return cdesc_().weights_desc.dims[2 + with_groups()]; }
    inline int KW() const
    { return cdesc_().weights_desc.dims[3 + with_groups()]; }

    inline int KSH() const { return cdesc_().strides[0]; }
    inline int KSW() const { return cdesc_().strides[1]; }

    inline int padT() const { return cdesc_().padding[0][0]; }
    inline int padB() const { return cdesc_().padding[1][0]; }
    inline int padL() const { return cdesc_().padding[0][1]; }
    inline int padR() const { return cdesc_().padding[1][1]; }

    inline double negative_slope() const;

    inline bool with_bias() const
    { return !memory_desc_wrapper(cdesc_().bias_desc).is_zero(); }
    inline bool with_groups() const
    { return cdesc_().weights_desc.ndims == cdesc_().src_desc.ndims + 1; }

protected:
    base_desc_t desc_;
    const _convolution_fwd_pd_t *hint_fwd_pd_;

    inline const convolution_desc_t &cdesc_() const;

    virtual status_t init() = 0;
};

using convolution_fwd_pd_t = mkldnn::impl::_convolution_fwd_pd_t<false>;
using convolution_relu_fwd_pd_t = mkldnn::impl::_convolution_fwd_pd_t<true>;

template<> inline double convolution_fwd_pd_t::negative_slope() const
{ return 0.; }
template<> inline double convolution_relu_fwd_pd_t::negative_slope() const
{ return desc()->negative_slope; }

template<>
inline const convolution_desc_t &convolution_fwd_pd_t::cdesc_() const
{ return desc_; }
template<>
inline const convolution_desc_t &convolution_relu_fwd_pd_t::cdesc_() const
{ return desc_.convolution_desc; }

}
}

#endif

// vim: et ts=4 sw=4 cindent cino^=l0,\:0,N-s
