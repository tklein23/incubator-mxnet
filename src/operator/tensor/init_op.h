/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*!
 *  Copyright (c) 2015 by Contributors
 * \file init_op.h
 * \brief Function definition of initialization op
 */
#ifndef MXNET_OPERATOR_TENSOR_INIT_OP_H_
#define MXNET_OPERATOR_TENSOR_INIT_OP_H_

#include <mxnet/base.h>
#include <mxnet/operator_util.h>
#include <mxnet/op_attr_types.h>
#include <mxnet/imperative.h>
#include <dmlc/parameter.h>
#include <dmlc/optional.h>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include "../mshadow_op.h"
#include "../elemwise_op_common.h"
#include "../mxnet_op.h"
#include "../mshadow_op.h"


namespace mxnet {
namespace op {

struct InitOpParam : public dmlc::Parameter<InitOpParam> {
  mxnet::TShape shape;
  std::string ctx;
  int dtype;
  DMLC_DECLARE_PARAMETER(InitOpParam) {
    DMLC_DECLARE_FIELD(shape)
    .set_default(mxnet::TShape(0, 1))
    .describe("The shape of the output");
    DMLC_DECLARE_FIELD(ctx)
    .set_default("")
    .describe("Context of output, in format [cpu|gpu|cpu_pinned](n)."
              "Only used for imperative calls.");
    DMLC_DECLARE_FIELD(dtype).set_default(mshadow::kFloat32)
    MXNET_ADD_ALL_TYPES
    .describe("Target data type.");
  }
};

struct InitOpWithoutDTypeParam : public dmlc::Parameter<InitOpWithoutDTypeParam> {
  mxnet::TShape shape;
  std::string ctx;
  int dtype;
  DMLC_DECLARE_PARAMETER(InitOpWithoutDTypeParam) {
    DMLC_DECLARE_FIELD(shape)
    .set_default(mxnet::TShape())
    .describe("The shape of the output");
    DMLC_DECLARE_FIELD(ctx)
    .set_default("")
    .describe("Context of output, in format [cpu|gpu|cpu_pinned](n)."
              "Only used for imperative calls.");
    DMLC_DECLARE_FIELD(dtype)
    .set_default(-1)
    .describe("Target data type.");
  }
};

struct EyeParam : public dmlc::Parameter<EyeParam> {
  nnvm::dim_t N;
  nnvm::dim_t M;
  nnvm::dim_t k;
  std::string ctx;
  int dtype;

  DMLC_DECLARE_PARAMETER(EyeParam) {
    DMLC_DECLARE_FIELD(N)
    .describe("Number of rows in the output.");
    DMLC_DECLARE_FIELD(M)
    .set_default(0)
    .describe("Number of columns in the output. If 0, defaults to N");
    DMLC_DECLARE_FIELD(k)
    .set_default(0)
    .describe("Index of the diagonal. 0 (the default) refers to the main diagonal."
              "A positive value refers to an upper diagonal."
              "A negative value to a lower diagonal.");
    DMLC_DECLARE_FIELD(ctx)
    .set_default("")
    .describe("Context of output, in format [cpu|gpu|cpu_pinned](n)."
              "Only used for imperative calls.");
    DMLC_DECLARE_FIELD(dtype).set_default(mshadow::kFloat32)
    .add_enum("float32", mshadow::kFloat32)
    .add_enum("float64", mshadow::kFloat64)
    .add_enum("float16", mshadow::kFloat16)
    .add_enum("uint8", mshadow::kUint8)
    .add_enum("int8", mshadow::kInt8)
    .add_enum("int32", mshadow::kInt32)
    .add_enum("int64", mshadow::kInt64)
    .describe("Target data type.");
  }
};

template<typename ParamType>
inline bool InitEyeShape(const nnvm::NodeAttrs& attrs,
                         mxnet::ShapeVector *in_attrs,
                         mxnet::ShapeVector *out_attrs) {
  const ParamType& param = nnvm::get<ParamType>(attrs.parsed);
  CHECK_EQ(in_attrs->size(), 0U);
  CHECK_EQ(out_attrs->size(), 1U);
  SHAPE_ASSIGN_CHECK(*out_attrs, 0, mshadow::Shape2(param.N, param.M > 0 ? param.M : param.N));
  return true;
}

template<int req>
struct eye_dns_fill {
  template<typename DType>
  MSHADOW_XINLINE static void Map(int i, DType* out_data,
                                  const nnvm::dim_t init_col,
                                  const nnvm::dim_t k,
                                  const nnvm::dim_t num_cols) {
    KERNEL_ASSIGN(out_data[(i+init_col-k)*num_cols+i+init_col], req, static_cast<DType>(1));
  }
};


struct RangeParam : public dmlc::Parameter<RangeParam> {
  double start;
  dmlc::optional<double> stop;
  double step;
  int repeat;
  bool infer_range;
  std::string ctx;
  int dtype;
  DMLC_DECLARE_PARAMETER(RangeParam) {
    DMLC_DECLARE_FIELD(start)
    .describe("Start of interval. The interval includes this value. The default start value is 0.");
    DMLC_DECLARE_FIELD(stop)
    .set_default(dmlc::optional<double>())
    .describe("End of interval. The interval does not include this value,"
              " except in some cases where step is not an integer and"
              " floating point round-off affects the length of out.");
    DMLC_DECLARE_FIELD(step)
    .set_default(1)
    .describe("Spacing between values.");
    DMLC_DECLARE_FIELD(repeat)
    .set_default(1)
    .describe("The repeating time of all elements."
              " E.g repeat=3, the element a will be repeated three times --> a, a, a.");
    DMLC_DECLARE_FIELD(infer_range)
    .set_default(false)
    .describe("When set to True, infer the stop position from the start, step, "
              "repeat, and output tensor size.");
    DMLC_DECLARE_FIELD(ctx)
    .set_default("")
    .describe("Context of output, in format [cpu|gpu|cpu_pinned](n)."
              "Only used for imperative calls.");
    DMLC_DECLARE_FIELD(dtype).set_default(mshadow::kFloat32)
    MXNET_ADD_ALL_TYPES
    .describe("Target data type.");
  }
};

/*! \brief Initialize and fill output with an arbitrary value */
struct InitOpWithScalarParam : dmlc::Parameter<InitOpWithScalarParam> {
  mxnet::TShape shape;
  std::string ctx;
  int dtype;
  double value;
  DMLC_DECLARE_PARAMETER(InitOpWithScalarParam) {
    DMLC_DECLARE_FIELD(shape)
      .set_default(mxnet::TShape())
      .describe("The shape of the output");
    DMLC_DECLARE_FIELD(ctx)
      .set_default("")
      .describe("Context of output, in format [cpu|gpu|cpu_pinned](n)."
                  "Only used for imperative calls.");
    DMLC_DECLARE_FIELD(dtype).set_default(mshadow::kFloat32)
      MXNET_ADD_ALL_TYPES
      .describe("Target data type.");
    DMLC_DECLARE_FIELD(value)
      .describe("Value with which to fill newly created tensor");
  }
};

/*! \brief Parse keyword arguments as PType arguments and save to parsed */
inline void RangeParamParser(nnvm::NodeAttrs* attrs) {
  RangeParam param;
  param.Init(attrs->dict);
  if (!static_cast<bool>(param.infer_range) && !static_cast<bool>(param.stop)) {
    param.stop = param.start;
    param.start = 0;
  }
  attrs->parsed = std::move(param);
}

struct LinspaceParam : public dmlc::Parameter<LinspaceParam> {
  double start;
  double stop;
  int num;
  bool endpoint;
  std::string ctx;
  int dtype;
  DMLC_DECLARE_PARAMETER(LinspaceParam) {
    DMLC_DECLARE_FIELD(start)
    .describe("The starting value of the sequence.");
    DMLC_DECLARE_FIELD(stop)
    .describe("The ending value of the sequence");
    DMLC_DECLARE_FIELD(num)
    .describe("Number of samples to generate. Must be non-negative.");
    DMLC_DECLARE_FIELD(endpoint)
    .set_default(true)
    .describe("If True, stop is the last sample. Otherwise, it is not included.");
    DMLC_DECLARE_FIELD(ctx)
    .set_default("")
    .describe("Context of output, in format [cpu|gpu|cpu_pinned](n)."
              "Only used for imperative calls.");
    DMLC_DECLARE_FIELD(dtype).set_default(mshadow::kFloat32)
    MXNET_ADD_ALL_TYPES
    .describe("Target data type.");
  }
};

template<typename ParamType>
inline bool InitShape(const nnvm::NodeAttrs& attrs,
                      mxnet::ShapeVector *in_attrs,
                      mxnet::ShapeVector *out_attrs) {
  const ParamType& param = nnvm::get<ParamType>(attrs.parsed);
  CHECK_EQ(in_attrs->size(), 0U);
  CHECK_EQ(out_attrs->size(), 1U);
  mxnet::TShape param_shape = param.shape;
  if (!Imperative::Get()->is_np_shape()) {
    common::ConvertToNumpyShape(&param_shape);
  }
  if (shape_is_known((*out_attrs)[0]) && !shape_is_known(param_shape)) return true;
  SHAPE_ASSIGN_CHECK(*out_attrs, 0, param_shape);
  return shape_is_known(out_attrs->at(0));
}

template<typename ParamType>
inline bool InitType(const nnvm::NodeAttrs& attrs,
                       std::vector<int> *in_attrs,
                       std::vector<int> *out_attrs) {
  const ParamType& param = nnvm::get<ParamType>(attrs.parsed);
  CHECK_EQ(in_attrs->size(), 0U);
  CHECK_EQ(out_attrs->size(), 1U);
  TYPE_ASSIGN_CHECK(*out_attrs, 0, param.dtype);
  return true;
}

template<typename ParamType, bool rsp, bool csr>
inline bool InitStorageType(const nnvm::NodeAttrs& attrs,
                            const int dev_mask,
                            DispatchMode* dispatch_mode,
                            std::vector<int> *in_attrs,
                            std::vector<int> *out_attrs) {
  CHECK_EQ(in_attrs->size(), 0U);
  CHECK_EQ(out_attrs->size(), 1U);
  auto &out_stype = out_attrs->at(0);
  bool dispatched = false;
  type_assign(&out_stype, kDefaultStorage);
  if (!dispatched && out_stype == kDefaultStorage) {
    // default
    dispatched = storage_type_assign(out_attrs, kDefaultStorage,
                                     dispatch_mode, DispatchMode::kFCompute);
  }
  if (!dispatched && rsp && out_stype == kRowSparseStorage) {
    // rsp
    dispatched = storage_type_assign(out_attrs, kRowSparseStorage,
                                     dispatch_mode, DispatchMode::kFComputeEx);
  }
  if (!dispatched && csr && out_stype == kCSRStorage) {
    // csr
    dispatched = storage_type_assign(out_attrs, kCSRStorage,
                                     dispatch_mode, DispatchMode::kFComputeEx);
  }
  if (!dispatched) {
    dispatched = dispatch_fallback(out_attrs, dispatch_mode);
  }
  return dispatched;
}

/*!
 * \brief General-purpose blob value-filling function
 * \tparam xpu cpu or gpu
 * \tparam ValueType Data type of supplied value
 * \tparam is_integer Whether to optimize for an integer value
 * \param s Stream
 * \param b The blob to fill with a value
 * \param req Request type (kNullOp, kWriteTo, etc)
 * \param val The value to use for the filling operation
 */
template <bool is_integer = false, typename ValueType, typename xpu>
void Fill(mshadow::Stream<xpu> *s, const TBlob& b, const OpReqType req, ValueType val) {
  // If b is a zero-size tensor, do nothing.
  if (b.Size() == 0) return;
  if (req != kNullOp) {
    const size_t size = b.Size();
    if (val == 0) {
      if (req != kAddTo) {
        if (b.dev_mask() == cpu::kDevMask && size < 50000) {
          MSHADOW_TYPE_SWITCH(b.type_flag_, DType, {
            memset(b.dptr_, 0, size * sizeof(DType));
          });
        } else {
          // Optimize common use-case of filling with ones
          MSHADOW_TYPE_SWITCH(b.type_flag_, DType, {
            MXNET_ASSIGN_REQ_SWITCH(req, Req, {
              mxnet_op::Kernel<mxnet_op::op_with_req<mxnet_op::set_to_int<0>, Req>, xpu>::Launch(
                s, b.Size(), b.dptr<DType>());
            });
          });
        }
      }
    } else if (is_integer && val == 1) {
      // Optimize common use-case of filling with ones
      MSHADOW_TYPE_SWITCH(b.type_flag_, DType, {
        MXNET_ASSIGN_REQ_SWITCH(req, Req, {
          mxnet_op::Kernel<mxnet_op::op_with_req<mxnet_op::set_one, Req>, xpu>::Launch(
            s, b.Size(), b.dptr<DType>());
        });
      });
    } else {
      // Generic fill kernel from variable
      MSHADOW_TYPE_SWITCH(b.type_flag_, DType, {
        MXNET_ASSIGN_REQ_SWITCH(req, Req, {
          mxnet_op::Kernel<mxnet_op::op_with_req<mshadow_op::identity, Req>, xpu>::Launch(
            s, b.Size(), b.dptr<DType>(), static_cast<DType>(val));
        });
      });
    }
  }
}

/*! \brief Fill output with a scalar integer value */
template<typename xpu, int value>
void FillCompute(const nnvm::NodeAttrs& attrs,
                 const OpContext& ctx,
                 const std::vector<TBlob>& inputs,
                 const std::vector<OpReqType>& req,
                 const std::vector<TBlob>& outputs) {
  Fill<true>(ctx.get_stream<xpu>(), outputs[0], req[0], value);
}

/*! \brief Fill output with an arbitrary value */
template<typename xpu>
void InitFillWithScalarCompute(const nnvm::NodeAttrs &attrs,
                               const OpContext &ctx,
                               const std::vector<TBlob> &inputs,
                               const std::vector<OpReqType> &req,
                               const std::vector<TBlob> &outputs) {
  CHECK_EQ(inputs.size(), 0);
  CHECK_EQ(outputs.size(), 1U);
  const auto& param = nnvm::get<InitOpWithScalarParam>(attrs.parsed);
  Fill<false>(ctx.get_stream<xpu>(), outputs[0], req[0], param.value);
}

struct PopulateFullIdxRspKernel : public mxnet_op::tunable {
  template<typename IType>
  MSHADOW_XINLINE static void Map(int i, IType* out) {
    KERNEL_ASSIGN(out[i], kWriteTo, i);
  }
};

// Fill in the indices and values of a RowSparse NDArray to represent a zeros NDArray,
// instead of the usual compact representation.
template<typename xpu>
inline void FillDnsZerosRspImpl(mshadow::Stream<xpu> *s, NDArray *dst) {
  using namespace rowsparse;
  using namespace mshadow::expr;
  using namespace mshadow;
  using namespace mxnet_op;
  CHECK_EQ(dst->storage_type(), kRowSparseStorage);
  MSHADOW_IDX_TYPE_SWITCH(dst->aux_type(kIdx), IType, {
    const index_t num_rows = dst->shape()[0];
    dst->CheckAndAlloc({Shape1(num_rows)});
    Fill<true>(s, dst->data(), kWriteTo, 0);
    auto idx = dst->aux_data(kIdx).FlatTo1D<xpu, IType>(s);
    Kernel<PopulateFullIdxRspKernel, xpu>::Launch(s, num_rows, idx.dptr_);
  });
}

/*!
 * \brief Fill a rsp NDArray with zeros by updating the aux shape.
 * \tparam xpu - cpu or gpu
 * \param s - The device stream
 * \param dst - NDArray which is to be set to "all zeroes"
 */
template<typename xpu>
void FillZerosRspImpl(mshadow::Stream<xpu> *, const NDArray& dst) {
  CHECK_EQ(dst.storage_type(), kRowSparseStorage) << "dst should be an RSP NDArray";
  if (dst.storage_initialized()) {
    // reset the shapes if it's not zeros (set_aux_shape() will set storage_shape to zero as well)
    dst.set_aux_shape(rowsparse::kIdx, mxnet::TShape(mshadow::Shape1(0)));
  }
}

/*!
 * \brief Fill a CSR NDArray with zeros by updating the aux shape
 * \param s - The device stream
 * \param dst - NDArray which is to be set to "all zeroes"
 */
inline void FillZerosCsrImpl(mshadow::Stream<mshadow::cpu> *s, const NDArray& dst) {
  CHECK_EQ(dst.storage_type(), kCSRStorage) << "dst is not a CSR NDArray";
  dst.set_aux_shape(csr::kIdx, mshadow::Shape1(0));
  dst.CheckAndAllocAuxData(csr::kIndPtr, mshadow::Shape1(dst.shape()[0] + 1));
  TBlob indptr_data = dst.aux_data(csr::kIndPtr);
  Fill<true>(s, dst.aux_data(csr::kIndPtr), kWriteTo, 0);
}
void FillZerosCsrImpl(mshadow::Stream<mshadow::gpu> *s, const NDArray& dst);

/*!
 * \brief Fill an NDArray with zeros
 * \tparam xpu - cpu or gpu
 * \param attrs  - node attributes (unused)
 * \param ctx - Device context
 * \param inputs - NDArray inputs (unused)
 * \param req - Request type (i.e. kWrite, kNullOp, etc.)
 * \param outputs - Array which contains at position zero (0) the array to be set to zeros
 */
template<typename xpu>
void FillComputeZerosEx(const nnvm::NodeAttrs& attrs,
                        const OpContext& ctx,
                        const std::vector<NDArray>& inputs,
                        const std::vector<OpReqType>& req,
                        const std::vector<NDArray>& outputs) {
  using namespace mshadow;
  using namespace mshadow::expr;
  Stream<xpu> *s = ctx.get_stream<xpu>();
  CHECK_EQ(outputs.size(), 1);
  auto stype = outputs[0].storage_type();
  // x + 0 == x
  if (req[0] == kNullOp || req[0] == kAddTo) return;
  if (stype == kRowSparseStorage) {
    FillZerosRspImpl(s, outputs[0]);
  } else if (stype == kCSRStorage) {
    FillZerosCsrImpl(s, outputs[0]);
  } else {
    LogUnimplementedOp(attrs, ctx, inputs, req, outputs);
  }
}


template<typename xpu>
void EyeFill(const nnvm::NodeAttrs& attrs,
             const OpContext& ctx,
             const std::vector<TBlob>& inputs,
             const std::vector<OpReqType>& req,
             const std::vector<TBlob>& outputs) {
  CHECK_EQ(inputs.size(), 0U);
  CHECK_EQ(outputs.size(), 1U);
  CHECK_EQ(req.size(), 1U);
  mshadow::Stream<xpu> *s = ctx.get_stream<xpu>();
  const EyeParam& param = nnvm::get<EyeParam>(attrs.parsed);
  const TBlob& out_data = outputs[0];
  const nnvm::dim_t num_cols = param.M > 0 ? param.M : param.N;

  const nnvm::dim_t cnnz = std::max(num_cols - std::abs(param.k), (nnvm::dim_t)0);
  const nnvm::dim_t rnnz = std::max(param.N - std::abs(param.k), (nnvm::dim_t)0);
  const nnvm::dim_t nnz = param.k > 0 ? std::min(cnnz, param.N) :
                                        std::min(rnnz, num_cols);
  using namespace mxnet_op;
  MSHADOW_TYPE_SWITCH(out_data.type_flag_, DType, {
    MXNET_ASSIGN_REQ_SWITCH(req[0], req_type, {
      Fill(s, out_data, req[0], static_cast<DType>(0));
      if (nnz > 0) {
        Kernel<eye_dns_fill<req_type>, xpu>::Launch(s, nnz, out_data.dptr<DType>(),
          std::max(static_cast<nnvm::dim_t>(0), param.k), param.k, num_cols);
      }
    });
  });
}


struct range_fwd {
  template<typename DType>
  MSHADOW_XINLINE static void Map(index_t i, index_t repeat, DType start, DType step,
                                  int req, DType* out) {
    KERNEL_ASSIGN(out[i], req, start + (i/repeat) * step);
  }
};

template<typename xpu>
void RangeCompute(const nnvm::NodeAttrs& attrs,
                  const OpContext& ctx,
                  const std::vector<TBlob>& inputs,
                  const std::vector<OpReqType>& req,
                  const std::vector<TBlob>& outputs) {
  using namespace mxnet_op;
  Stream<xpu> *s = ctx.get_stream<xpu>();
  const RangeParam& param = nnvm::get<RangeParam>(attrs.parsed);
  MSHADOW_TYPE_SWITCH(outputs[0].type_flag_, DType, {
      // Force unsigned params to take two's complement form on ARM to ensure consistency with x86
      // results.  Casting negative floats to unsigned types is undefined in the CPP standard.
      auto step = std::is_signed<DType>() ? param.step : static_cast<index_t>(param.step);
      auto start = std::is_signed<DType>() ? param.start : static_cast<index_t>(param.start);
      Kernel<range_fwd, xpu>::Launch(s,
                                     outputs[0].Size(),
                                     static_cast<int>(param.repeat),
                                     static_cast<DType>(start),
                                     static_cast<DType>(step),
                                     req[0],
                                     outputs[0].dptr<DType>());
  });
}


inline bool RangeShape(const nnvm::NodeAttrs& attrs,
                       mxnet::ShapeVector *in_attrs,
                       mxnet::ShapeVector *out_attrs) {
  const RangeParam& param = nnvm::get<RangeParam>(attrs.parsed);
  CHECK_EQ(in_attrs->size(), 0U);
  CHECK_EQ(out_attrs->size(), 1U);
  CHECK_NE(param.step, 0)
    << "Range does not support step=0, received " << param.step;
  CHECK(param.repeat > 0)
    << "Range only supports repeat > 0, received " << param.repeat;
  if (param.infer_range && !param.stop.has_value()) {
    return false;
  }
  if (param.step > 0) {
    CHECK(param.start < param.stop.value())
      << "Invalid range (start, stop, step) = "
      << "(" << param.start << "," << param.stop.value() << "," << param.step << ")";
  } else {
    CHECK(param.start > param.stop.value())
      << "Invalid range (start, stop, step)= "
      << "(" << param.start << "," << param.stop.value() << "," << param.step << ")";
  }
  const double out_size = std::ceil((param.stop.value() - param.start) / param.step)
                          * param.repeat;
  SHAPE_ASSIGN_CHECK(*out_attrs, 0, mxnet::TShape({static_cast<nnvm::dim_t>(out_size)}));
  return true;
}

struct linspace_fwd {
  template<typename DType>
  MSHADOW_XINLINE static void Map(index_t i, double start, double stop, double step,
                                  int req, DType* out) {
    KERNEL_ASSIGN(out[i], req, static_cast<DType>(start + step * i));
  }
};

template<typename xpu>
void LinspaceCompute(const nnvm::NodeAttrs& attrs,
                     const OpContext& ctx,
                     const std::vector<TBlob>& inputs,
                     const std::vector<OpReqType>& req,
                     const std::vector<TBlob>& outputs) {
  using namespace mxnet_op;
  Stream<xpu> *s = ctx.get_stream<xpu>();
  const LinspaceParam& param = nnvm::get<LinspaceParam>(attrs.parsed);
  MSHADOW_TYPE_SWITCH(outputs[0].type_flag_, DType, {
      int step_num = param.endpoint ? param.num - 1 : param.num;
      double step = step_num > 0 ? (param.stop - param.start) / step_num : 0.0f;
      Kernel<linspace_fwd, xpu>::Launch(s,
                                        outputs[0].Size(),
                                        param.start,
                                        param.stop,
                                        step,
                                        req[0],
                                        outputs[0].dptr<DType>());
  });
}

inline bool LinspaceShape(const nnvm::NodeAttrs& attrs,
                       mxnet::ShapeVector *in_attrs,
                       mxnet::ShapeVector *out_attrs) {
  const LinspaceParam& param = nnvm::get<LinspaceParam>(attrs.parsed);
  CHECK_EQ(in_attrs->size(), 0U);
  CHECK_EQ(out_attrs->size(), 1U);
  CHECK_GE(param.num, 0)
    << "Number of sequence should be non-negative, received " << param.num;
  SHAPE_ASSIGN_CHECK(*out_attrs, 0, mxnet::TShape({static_cast<nnvm::dim_t>(param.num)}));
  return true;
}

}  // namespace op
}  // namespace mxnet

#endif  // MXNET_OPERATOR_TENSOR_INIT_OP_H_
