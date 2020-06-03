#pragma once

#include "gui/IGui.hpp"

#include "asset/Asset.hpp"

FORWARD_DEF(NS_MEMORY, class MemoryChunk)

NS_GUI

class AssetEditor : public IGui
{

public:
	AssetEditor() = default;
	
	virtual void setAsset(asset::AssetPtrStrong asset);

	i32 getFlags() const override;
	virtual void makeGui() override;
	virtual void renderView() override;

protected:

	std::string getId() const override;
	std::string getTitle() const override;

	virtual bool isAssetDirty() const;
	void markAssetDirty(ui32 bit, bool isDirty = true);
	virtual void saveAsset();
	void releaseAsset();

	template <typename TAsset>
	std::shared_ptr<TAsset> get() const
	{
		return std::dynamic_pointer_cast<TAsset>(this->mpAsset);
	}
	
	void renderMenuBar();

private:
	asset::AssetPtrStrong mpAsset;
	// If any bits are set, then the asset is dirty
	ui32 mDirtyFlags;

};

NS_END
