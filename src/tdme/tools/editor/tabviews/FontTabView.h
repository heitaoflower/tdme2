#pragma once

#include <string>

#include <tdme/tdme.h>
#include <tdme/engine/fwd-tdme.h>
#include <tdme/gui/nodes/fwd-tdme.h>
#include <tdme/tools/editor/misc/PopUps.h>
#include <tdme/tools/editor/tabcontrollers/fwd-tdme.h>
#include <tdme/tools/editor/tabcontrollers/FontTabController.h>
#include <tdme/tools/editor/tabcontrollers/TabController.h>
#include <tdme/tools/editor/tabviews/fwd-tdme.h>
#include <tdme/tools/editor/tabviews/TabView.h>
#include <tdme/tools/editor/views/fwd-tdme.h>

using std::string;

using tdme::engine::Engine;
using tdme::engine::FrameBuffer;
using tdme::gui::nodes::GUIScreenNode;
using tdme::tools::editor::misc::PopUps;
using tdme::tools::editor::tabcontrollers::FontTabController;
using tdme::tools::editor::tabcontrollers::TabController;
using tdme::tools::editor::tabviews::TabView;
using tdme::tools::editor::views::EditorView;

/**
 * Font tab view
 * @author Andreas Drewke
 */
class tdme::tools::editor::tabviews::FontTabView final
	: public TabView
{
protected:
	Engine* engine { nullptr };

private:
	EditorView* editorView { nullptr };
	string tabId;
	GUIScreenNode* screenNode { nullptr };
	PopUps* popUps { nullptr };
	FontTabController* fontTabController { nullptr };
	TabView::OutlinerState outlinerState;

public:
	/**
	 * Public constructor
	 * @param editorView editor view
	 * @param tabId tab id
	 * @param screenNode screenNode
	 */
	FontTabView(EditorView* editorView, const string& tabId, GUIScreenNode* screenNode);

	/**
	 * Destructor
	 */
	~FontTabView();

	/**
	 * @return editor view
	 */
	inline EditorView* getEditorView() {
		return editorView;
	}

	/**
	 * @return associated tab controller
	 */
	inline TabController* getTabController() override {
		return fontTabController;
	}

	/**
	 * @return pop up views
	 */
	inline PopUps* getPopUps() {
		return popUps;
	}

	// overridden methods
	void handleInputEvents() override;
	void display() override;
	inline const string& getTabId() override {
		return tabId;
	}
	void initialize() override;
	void dispose() override;
	Engine* getEngine() override;
	void activate() override;
	void deactivate() override;
	void reloadOutliner() override;
	void updateRendering() override;
	inline bool hasFixedSize() override{ return false; };

};
