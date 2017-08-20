/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserve.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include <paddle/framework/op_registry.h>

#include <vector>

namespace paddle {
namespace framework {

std::unique_ptr<OperatorBase> OpRegistry::CreateOp(
    const std::string& type, const VariableNameMap& inputs,
    const VariableNameMap& outputs, AttributeMap attrs) {
  auto it = OpInfoMap().find(type);
  PADDLE_ENFORCE(it != OpInfoMap().end(),
                 "Operator '%s' has not been registered.", type);
  it->second.checker_->Check(attrs);
  auto op = it->second.creator_(type, inputs, outputs, attrs);
  return std::unique_ptr<OperatorBase>(op);
}

static VariableNameMap ConvertOpDescVarsToVarNameMap(
    const google::protobuf::RepeatedPtrField<OpDesc::Var>& op_desc_vars) {
  VariableNameMap ret_val;
  for (auto& var : op_desc_vars) {
    auto& var_names = ret_val[var.parameter()];
    auto& var_names_in_proto = var.arguments();
    var_names.reserve(static_cast<size_t>(var_names_in_proto.size()));
    std::copy(var_names_in_proto.begin(), var_names_in_proto.end(),
              std::back_inserter(var_names));
  }
  return ret_val;
}

std::unique_ptr<OperatorBase> OpRegistry::CreateOp(const OpDesc& op_desc) {
  VariableNameMap inputs = ConvertOpDescVarsToVarNameMap(op_desc.inputs());
  VariableNameMap outputs = ConvertOpDescVarsToVarNameMap(op_desc.outputs());
  AttributeMap attrs;
  for (auto& attr : op_desc.attrs()) {
    attrs[attr.name()] = GetAttrValue(attr);
  }

  return CreateOp(op_desc.type(), inputs, outputs, attrs);
}

std::unique_ptr<OperatorBase> OpRegistry::CreateGradOp(const OperatorBase& op) {
  PADDLE_ENFORCE(!op.IsNetOp(), "Use framework::Backward to get backward ops");
  return std::unique_ptr<OperatorBase>(BuildGradOp(&op));
}

}  // namespace framework
}  // namespace paddle
