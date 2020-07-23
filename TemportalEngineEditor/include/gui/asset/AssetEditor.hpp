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

protected:

	std::string getId() const override;
	std::string getTitle() const override;

	virtual void renderView() override;
	virtual f32 getDetailsPanelWidth() const { return 0; }
	virtual void renderDetailsPanel() {};
	virtual void renderContent();

	ui32 getDirtyFlags() const;
	virtual bool isAssetDirty() const;
	bool isBitDirty(ui32 bit) const;
	void markAssetDirty(ui32 bit, bool isDirty = true);
	void markAssetClean();

	virtual void saveAsset();
	bool hasCompiledBinary() const;
	virtual bool canCompileAsset();
	virtual void compileAsset();
	virtual void onBuildFailure(std::vector<std::string> const &errors);
	bool shouldReleaseGui() const override;

	template <typename TAsset>
	std::shared_ptr<TAsset> get() const
	{
		return std::dynamic_pointer_cast<TAsset>(this->mpAsset);
	}
	
	void renderMenuBar();
	virtual void renderMenuBarItems();
	void renderBinaryInformation();

private:
	asset::AssetPtrStrong mpAsset;
	bool mbDetailsPanelOpen;
	// If any bits are set, then the asset is dirty
	ui32 mDirtyFlags;
	bool mbIsBuildingAsset;

};

NS_END
