#pragma once

#include "property/Base.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/RenderPassAsset.hpp"

NS_PROPERTIES

DECLARE_PROPERTY_EDITOR(asset::Pipeline::DescriptorGroup);
DECLARE_PROPERTY_EDITOR(asset::Pipeline::Descriptor);
DECLARE_PROPERTY_EDITOR(asset::RenderPass::DepthStencil);

NS_END
