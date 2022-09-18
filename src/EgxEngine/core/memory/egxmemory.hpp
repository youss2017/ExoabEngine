#pragma once
#include "../egxcommon.hpp"
#include "egxref.hpp"
#include <memory>
#include <imgui.h>

namespace egx {

	enum class memorylayout : uint32_t {
		local,
		dynamic,
		stream
	};

	enum buffertype : uint32_t {
		buffertype_vertex = 0b0001,
		buffertype_index = 0b0010,
		buffertype_storage = 0b0100,
		buffertype_uniform = 0b1000,
		buffertype_onlytransfer = 0b0000
	};

	class Buffer {

	public:
		static ref<Buffer> EGX_API FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface, size_t size, memorylayout layout, buffertype type, bool requireCoherent = false);

		EGX_API ~Buffer();
		EGX_API Buffer(Buffer& cp) = delete;
		EGX_API Buffer(Buffer&& move) noexcept;
		EGX_API Buffer& operator=(Buffer& move) noexcept;

		EGX_API ref<Buffer> clone();
		EGX_API ref<Buffer> cloneAndCopy();
		EGX_API void copy(ref<Buffer>& source);
		EGX_API void copy(Buffer* source);

		EGX_API void write(void* data, size_t offset, size_t size);
		EGX_API void write(void* data, size_t size);
		EGX_API void write(void* data);

		EGX_API void* map();
		EGX_API void unmap();

		EGX_API void read(void* pOutput, size_t offset, size_t size);
		EGX_API void read(void* pOutput, size_t size);
		EGX_API void read(void* pOutput);

		EGX_API void flush();
		EGX_API void flush(size_t offset, size_t size);
		EGX_API void invalidate();
		EGX_API void invalidate(size_t offset, size_t size);

	protected:
		EGX_API Buffer(
			const size_t size,
			const memorylayout layout,
			const buffertype type,
			const bool coherent,
			const VkBuffer buffer,
			const VkAlloc::CONTEXT context,
			const VkAlloc::BUFFER _buffer,
			const ref<VulkanCoreInterface>& coreinterface) :
			Size(size), Layout(layout),
			Type(type), CoherentFlag(coherent),
			Buf(buffer), _context(context),
			_buffer(_buffer), _coreinterface(coreinterface),
			_ptr(nullptr)
		{}

		void EGX_API copy(VkBuffer src, size_t offset, size_t size);

	public:
		const size_t Size;
		const memorylayout Layout;
		const buffertype Type;
		const bool CoherentFlag;
		const VkBuffer Buf;
	protected:
		const VkAlloc::CONTEXT _context;
		const VkAlloc::BUFFER _buffer;
		ref<VulkanCoreInterface> _coreinterface;
		void* _ptr;
	};

	class Image {

	public:
		/// <summary>
		/// Creates a image or texture
		/// </summary>
		/// <param name="CoreInterface">CoreInterfance from EngineCore</param>
		/// <param name="width">Width (or size for 1D Image)</param>
		/// <param name="height">Height (If Image is 1D Set to 1)</param>
		/// <param name="depth">Depth for 3D Images (for 2D/1D set to 1)</param>
		/// <param name="format">Format of image</param>
		/// <param name="mipcount">Set to 0 for max mipmap count</param>
		/// <param name="arraylevel">For arrayed images</param>
		/// <returns></returns>
		static ref<Image> EGX_API FactoryCreate(
			const ref<VulkanCoreInterface>& CoreInterface,
			memorylayout layout,
			VkImageAspectFlags aspect,
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			VkFormat format,
			uint32_t mipcount,
			uint32_t arraylevel,
			VkImageUsageFlags usage,
			VkImageLayout InitalLayout);

		EGX_API ~Image() noexcept;

		EGX_API Image(Image& cp) = delete;
		EGX_API Image(Image&& move) noexcept :
			Width(move.Width), Height(move.Height),
			Depth(move.Depth), Mipcount(move.Mipcount),
			Arraylevels(move.Arraylevels), Img(move.Img),
			ImageUsage(move.ImageUsage), _context(move._context),
			_image(move._image), _coreinterface(move._coreinterface),
			ImageAspect(move.ImageAspect), Format(move.Format) {
			memset(&move, 0, sizeof(Image));
		}
		EGX_API Image& operator=(Image& move) {
			this->~Image();
			memcpy(this, &move, sizeof(Image));
			memset(&move, 0, sizeof(Image));
			return *this;
		}

		EGX_API void setlayout(VkImageLayout OldLayout, VkImageLayout NewLayout);

		EGX_API void write(
			uint8_t* Data,
			VkImageLayout CurrentLayout,
			uint32_t xOffset,
			uint32_t yOffset,
			uint32_t zOffset,
			uint32_t Width,
			uint32_t Height,
			uint32_t Depth,
			uint32_t ArrayLevel,
			uint32_t StrideSizeInBytes);

		EGX_API void write(uint8_t* Data, VkImageLayout CurrentLayout, uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t ArrayLevel, uint32_t StrideInBytes);
		EGX_API void write(uint8_t* Data, VkImageLayout CurrentLayout, uint32_t Width, uint32_t Height, uint32_t ArrayLevel, uint32_t StrideInBytes);
		EGX_API void write(uint8_t* Data, VkImageLayout CurrentLayout, uint32_t Width, uint32_t ArrayLevel, uint32_t StrideInBytes);

		EGX_API void generatemipmap(VkImageLayout CurrentLayout, uint32_t ArrayLevel = 0);

		EGX_API VkImageView createview(
			uint32_t ViewId,
			uint32_t Miplevel,
			uint32_t MipCount,
			uint32_t ArrayLevel,
			uint32_t ArrayCount);

		EGX_API VkImageView createview(uint32_t ViewId);

		inline const VkImageView view(uint32_t ViewId) noexcept {
			assert(_views.find(ViewId) != _views.end());
			return _views[ViewId];
		}

		static egx::ref<egx::Image> EGX_API CreateCubemap(ref<VulkanCoreInterface>& CoreInterface, std::string_view path, VkFormat format);
		static ref<Image> EGX_API LoadFromDisk(ref<VulkanCoreInterface>& CoreInterface, std::string_view path, VkImageUsageFlags usage, VkImageLayout InitalLayout);

		void EGX_API barrier(VkCommandBuffer cmd, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			VkAccessFlags srcAccess, VkAccessFlags dstAccess,
			uint32_t miplevel = 0,
			uint32_t arraylevel = 0,
			uint32_t mipcount = VK_REMAINING_MIP_LEVELS,
			uint32_t arraycount = VK_REMAINING_ARRAY_LAYERS) const;

		VkImageMemoryBarrier EGX_API barrier(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess,
			uint32_t miplevel = 0,
			uint32_t arraylevel = 0,
			uint32_t mipcount = VK_REMAINING_MIP_LEVELS,
			uint32_t arraycount = VK_REMAINING_ARRAY_LAYERS) const;

		inline static VkImageMemoryBarrier Barrier(
			VkImage Img, 
			VkImageAspectFlags ImageAspect, 
			VkImageLayout oldLayout, 
			VkImageLayout newLayout, 
			VkAccessFlags srcAccess, 
			VkAccessFlags dstAccess,
			uint32_t miplevel = 0,
			uint32_t arraylevel = 0,
			uint32_t mipcount = VK_REMAINING_MIP_LEVELS,
			uint32_t arraycount = VK_REMAINING_ARRAY_LAYERS) {
			VkImageMemoryBarrier barr{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			barr.srcAccessMask = srcAccess;
			barr.dstAccessMask = dstAccess;
			barr.oldLayout = oldLayout;
			barr.newLayout = newLayout;
			barr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barr.image = Img;
			barr.subresourceRange.aspectMask = ImageAspect;
			barr.subresourceRange.baseMipLevel = miplevel;
			barr.subresourceRange.levelCount = mipcount;
			barr.subresourceRange.baseArrayLayer = arraylevel;
			barr.subresourceRange.layerCount = arraycount;
			return barr;
		}

		void EGX_API read(void* buffer, VkOffset3D offset, VkExtent3D size);

		/// <summary>
		/// Clones image with the same properties and same content.
		/// </summary>
		/// <param name="CurrentLayout">The current layout of the source image.</param>
		/// <param name="NewCopyFinalLayout">The final layout of new copied image, if undefinied it will be same as 'CurrentLayout'</param>
		/// <returns></returns>
		ref<Image> EGX_API copy(VkImageLayout CurrentLayout, VkImageLayout CopyFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED) const;
		// Same properties
		ref<Image> EGX_API clone() const;

		EGX_API ImTextureID GetImGuiTextureID(VkSampler sampler, uint32_t viewId = 0);

	protected:
		EGX_API Image(uint32_t width, uint32_t height,
			uint32_t depth, uint32_t mipcount, uint32_t arraylevels,
			VkImage image, VkImageUsageFlags usage,
			VkAlloc::CONTEXT _context, VkAlloc::IMAGE _image,
			const ref<VulkanCoreInterface>& _interface, VkFormat format,
			VkImageAspectFlags aspect, memorylayout layout,
			VkImageUsageFlags imageUsage, VkImageLayout initalLayout)
			: Width(width), Height(height),
			Depth(depth), Mipcount(mipcount),
			Arraylevels(arraylevels), Img(image),
			ImageUsage(usage), _context(_context),
			_image(_image), _coreinterface(_interface),
			Format(format), ImageAspect(aspect),
			_memorylayout(layout), _imageusage(imageUsage),
			_initallayout(initalLayout)
		{}

	public:
		const uint32_t Width;
		const uint32_t Height;
		const uint32_t Depth;
		const uint32_t Mipcount;
		const uint32_t Arraylevels;
		const VkImage Img;
		const VkImageUsageFlags ImageUsage;
		const VkFormat Format;
		const VkImageAspectFlags ImageAspect;

	protected:
		const VkAlloc::CONTEXT _context;
		const VkAlloc::IMAGE _image;
		ref<VulkanCoreInterface> _coreinterface;
		std::map<uint32_t, VkImageView> _views;
		ImTextureID _imgui_textureid = nullptr;
		memorylayout _memorylayout;
		VkImageUsageFlags _imageusage;
		VkImageLayout _initallayout;
	};

}