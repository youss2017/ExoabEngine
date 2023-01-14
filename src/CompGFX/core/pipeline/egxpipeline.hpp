#pragma once
#include "../egxcommon.hpp"
#include "../shaders/egxshader2.hpp"
#include "../shaders/egxshaderset.hpp"
#include "../memory/egxref.hpp"
#include "egxframebuffer.hpp"
#include <set>

namespace egx {

	enum cullmode : uint32_t {
		cullmode_none,
		cullmode_back,
		cullmode_front,
		cullmode_front_and_back
	};

	enum frontface : uint32_t {
		frontface_cw,
		frontface_ccw
	};

	enum depthcompare : uint32_t {
		depthcompare_never,
		depthcompare_less,
		depthcompare_equal,
		depthcompare_less_equal,
		depthcompare_greater,
		depthcompare_not_equal,
		depthcompare_greater_equal,
		depthcompare_always,
	};

	enum polygonmode : uint32_t {
		polygonmode_point,
		polygonmode_line,
		polygonmode_fill
	};

	enum polygontopology : uint32_t {
		polgyontopology_trianglelist,
		polgyontopology_linelist,
		polgyontopology_linestrip,
		polgyontopology_pointlist,
	};

	class Pipeline {
	public:
		static ref<Pipeline> EGX_API FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface);
		EGX_API ~Pipeline();
		EGX_API Pipeline(Pipeline& copy) = delete;
		EGX_API Pipeline(Pipeline&& move) noexcept;
		EGX_API Pipeline& operator=(Pipeline&& move) noexcept;

		/// <summary>
		/// PassId is determined from Framebuffer::CreatePass()
		/// </summary>
		/// <param name="layout"></param>
		/// <param name="vertex"></param>
		/// <param name="fragment"></param>
		/// <param name="framebuffer"></param>
		/// <param name="vertexDescription"></param>
		/// <returns></returns>
		void EGX_API Create(
			const ref<Shader2>& vertex,
			const ref<Shader2>& fragment,
			const ref<Framebuffer>& framebuffer,
			const uint32_t PassId);

		void EGX_API Create(const ref<Shader2>& compute);

		inline void Bind(VkCommandBuffer cmd) const {
			vkCmdBindPipeline(cmd, _graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline_);
			Layout->Bind(cmd, _graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE);
		}

		inline void PushConstants(VkCommandBuffer cmd, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, void* pData) {
			vkCmdPushConstants(cmd, Layout->GetLayout(), stageFlags, offset, size, pData);
		}

		inline VkPipeline operator()() const { return Pipeline_; }
		inline VkPipeline operator*() const { return Pipeline_; }

		inline void SetBuffer(uint32_t setId, uint32_t bindingId, const ref<Buffer>& buffer, uint32_t offset = 0, uint32_t structSize = 0) {
			Sets[setId]->SetBuffer(bindingId, buffer, structSize, offset);
		}

		inline void SetSampledImage(uint32_t setId, uint32_t bindingId, const egx::ref<Sampler>& sampler, const egx::ref<Image>& image, VkImageLayout imageLayout, uint32_t viewId) {
			Sets[setId]->SetImage(bindingId, { image }, { sampler }, { imageLayout }, { viewId });
		}

		inline void SetStorageImage(uint32_t setId, uint32_t bindingId, const egx::ref<Image>& image, VkImageLayout imageLayout, uint32_t viewId) {
			Sets[setId]->SetImage(bindingId, { image }, {}, { imageLayout }, { viewId });
		}

		// For imageLayouts, samplers, and viewIds if they only contain one element, then the first element is used for all images
		// otherwise each element in the vector is used per image in order
		inline void SetSampledImages(uint32_t setId, uint32_t bindingId, const std::vector<egx::ref<Sampler>>& samplers, 
			const std::vector<egx::ref<Image>>& images, const std::vector<VkImageLayout>& imageLayouts, const std::vector<uint32_t>& viewIds) {
			Sets[setId]->SetImage(bindingId, images, samplers, imageLayouts, viewIds);
		}

		// For imageLayouts and viewIds if they only contain one element, then the first element is used for all images
		// otherwise each element in the vector is used per image in order
		inline void SetStorageImages(uint32_t setId, uint32_t bindingId, const std::vector<egx::ref<Image>>& images, const std::vector<VkImageLayout>& imageLayouts, const std::vector<uint32_t>& viewIds) {
			Sets[setId]->SetImage(bindingId, images, {}, imageLayouts, viewIds);
		}

	protected:
		Pipeline(const ref<VulkanCoreInterface>& coreinterface) :
			_coreinterface(coreinterface) {}

	public:
		/// Pipeline properties you do not have to set viewport width/height
		// the default is to use the framebuffer
		cullmode CullMode = cullmode_none;
		frontface FrontFace = frontface_ccw;
		depthcompare DepthCompare = depthcompare_always;
		polygonmode FillMode = polygonmode_fill;
		polygontopology Topology = polgyontopology_trianglelist;
		float NearField = 0.0f;
		float FarField = 1.0f;
		float LineWidth = 1.0f;
		uint32_t ViewportWidth = 0;
		uint32_t ViewportHeight = 0;
		VkPipeline Pipeline_ = nullptr;
		ref<SetPool> Pool;
		ref<PipelineLayout> Layout;
		bool DepthEnabled = false;
		bool DepthWriteEnable = true;

	protected:
		std::map<uint32_t, ref<DescriptorSet>> Sets;
		ref<VulkanCoreInterface> _coreinterface;
		bool _graphics = false;

	};

}