/*
 * MenuItem.h
 *
 *  Created on: 7 May 2018
 *      Author: David
 */

#ifndef SRC_DISPLAY_MENUITEM_H_
#define SRC_DISPLAY_MENUITEM_H_

#include "General/FreelistManager.h"
#include "RepRapFirmware.h"
#include "ST7920/lcd7920.h"
#include "Storage/MassStorage.h"

// Menu item class hierarchy
class MenuItem
{
public:
	typedef uint8_t Alignment;
	typedef uint8_t FontNumber;
	typedef uint8_t Visibility;

	static constexpr Alignment LeftAlign = 0, CentreAlign = 1, RightAlign = 2;
	static constexpr Visibility AlwaysVisible = 0;

	// Draw this element on the LCD respecting 'maxWidth' and 'highlight'
	virtual void Draw(Lcd7920& lcd, PixelNumber maxWidth, bool highlight, PixelNumber tOffset) = 0;

	// Select this element with a push of the encoder.
	// If it returns nullptr then go into adjustment mode, if we can adjust the item.
	// Else execute the returned command.
	virtual const char* Select() { return nullptr; }

	// Actions to be taken when the menu system selects this item
	virtual void Enter(bool forwardDirection) {};

	// Actions to be taken when the menu system receives encoder counts and this item is currently selected
	// TODO: may be able to merge down with Adjust()
	virtual int Advance(int nCounts) { return nCounts; }

	// Return true if we can select this element for adjustment
	virtual bool CanAdjust() { return false; }

	// Adjust this element, returning true if we have finished adjustment.
	// 'clicks' is the number of encoder clicks to adjust by, or 0 if the button was pushed.
	virtual bool Adjust(int clicks) { return true; }

	// If the width was specified as zero, update it with the actual width
	virtual void UpdateWidth(Lcd7920& lcd) { }

	virtual ~MenuItem() { }

	MenuItem *GetNext() const { return next; }
	FontNumber GetFontNumber() const { return fontNumber; }
	void SetChanged() { itemChanged = true; }
	bool IsVisible() const;

	virtual PixelNumber GetVisibilityRowOffset(PixelNumber tCurrentOffset, PixelNumber fontHeight) const { return 0; }

	// Return the width of this item in pixels
	PixelNumber GetWidth() const { return width; }

	static void AppendToList(MenuItem **root, MenuItem *item);

protected:
	MenuItem(PixelNumber r, PixelNumber c, PixelNumber w, Alignment a, FontNumber fn, Visibility v);

	// Print the item starting at the current cursor position, which may be off screen. Used to find the width and also to really print the item.
	// Overridden for items that support variable alignment
	virtual void CorePrint(Lcd7920& lcd) { }

	// Print the item at the correct place with the correct alignment
	void PrintAligned(Lcd7920& lcd, PixelNumber tOffset, PixelNumber rightMargin);

	const PixelNumber row, column;
	PixelNumber width;
	const Alignment align;
	const FontNumber fontNumber;
	const Visibility visCase;

	bool itemChanged;
	bool highlighted;

private:
	MenuItem *next;
};

class TextMenuItem : public MenuItem
{
public:
	void* operator new(size_t sz) { return Allocate<TextMenuItem>(); }
	void operator delete(void* p) { Release<TextMenuItem>(p); }

	TextMenuItem(PixelNumber r, PixelNumber c, PixelNumber w, Alignment a, FontNumber fn, Visibility vis, const char *t);
	void Draw(Lcd7920& lcd, PixelNumber maxWidth, bool highlight, PixelNumber tOffset) override;
	void UpdateWidth(Lcd7920& lcd) override;

protected:
	void CorePrint(Lcd7920& lcd) override;

private:
	const char *text;
};

class ButtonMenuItem : public MenuItem
{
public:
	void* operator new(size_t sz) { return Allocate<ButtonMenuItem>(); }
	void operator delete(void* p) { Release<ButtonMenuItem>(p); }

	ButtonMenuItem(PixelNumber r, PixelNumber c, PixelNumber w, FontNumber fn, Visibility vis, const char *t, const char *cmd, const char *acFile);
	void Draw(Lcd7920& lcd, PixelNumber maxWidth, bool highlight, PixelNumber tOffset) override;
	void UpdateWidth(Lcd7920& lcd) override;
	const char* Select() override;

	PixelNumber GetVisibilityRowOffset(PixelNumber tCurrentOffset, PixelNumber fontHeight) const override;

protected:
	void CorePrint(Lcd7920& lcd) override;

private:
	const char *text;
	const char *command;
	const char *m_acFile; // used when action ("command") is "menu"

	// Scratch -- consumer is required to use as soon as it's returned
	// NOT THREAD SAFE!
	String<MaxFilenameLength + 20> m_acCommand; // TODO fix to proper max length
};

class ValueMenuItem : public MenuItem
{
public:
	void* operator new(size_t sz) { return Allocate<ValueMenuItem>(); }
	void operator delete(void* p) { Release<ValueMenuItem>(p); }

	ValueMenuItem(PixelNumber r, PixelNumber c, PixelNumber w, Alignment a, FontNumber fn, Visibility vis, bool adj, unsigned int v, unsigned int d);
	void Draw(Lcd7920& lcd, PixelNumber maxWidth, bool highlight, PixelNumber tOffset) override;
	const char* Select() override;
	bool CanAdjust() override { return true; }
	bool Adjust(int clicks) override;

	PixelNumber GetVisibilityRowOffset(PixelNumber tCurrentOffset, PixelNumber fontHeight) const override;

	unsigned int GetReferencedToolNumber() const;

protected:
	void CorePrint(Lcd7920& lcd) override;

private:
	enum class AdjustMode : uint8_t { displaying, adjusting, liveAdjusting };

	bool Adjust_SelectHelper();
	bool Adjust_AlterHelper(int clicks);

	static constexpr PixelNumber DefaultWidth =  25;			// default numeric field width

	const unsigned int valIndex;
	float currentValue;
	const char *textValue;				// for temporary use when printing
	uint8_t decimals;
	AdjustMode adjusting;
	bool adjustable;
	bool error;							// for temporary use when printing
};

class FilesMenuItem : public MenuItem
{
public:
	void* operator new(size_t sz) { return Allocate<FilesMenuItem>(); }
	void operator delete(void* p) { Release<FilesMenuItem>(p); }

	FilesMenuItem(PixelNumber r, PixelNumber c, PixelNumber w, FontNumber fn, Visibility vis, const char *cmd, const char *dir, const char *acFile, unsigned int nf);
	void Draw(Lcd7920& lcd, PixelNumber rightMargin, bool highlight, PixelNumber tOffset) override;
	void Enter(bool bForwardDirection) override;
	int Advance(int nCounts) override;
	const char* Select() override;

	PixelNumber GetVisibilityRowOffset(PixelNumber tCurrentOffset, PixelNumber fontHeight) const override;

	void EnterDirectory();

protected:
	void vResetViewState();

private:
	const unsigned int numDisplayLines;

	const char *command;
	const char *initialDirectory;
	const char *m_acFile; // used when action ("command") includes "menu"

	// Working
	String<MaxFilenameLength> m_acCurrentDirectory;

	bool bInSubdirectory() const;
	unsigned int uListingEntries() const;

	// Scratch -- consumer is required to use as soon as it's returned
	// NOT THREAD SAFE!
	String<MaxFilenameLength + 20> m_acCommand; // TODO fix to proper max length

	// Files on the file system, real count i.e. no ".." included
	unsigned int m_uHardItemsInDirectory;

	// Logical items (c. files) for display, referenced to uListingEntries() count
	unsigned int m_uListingFirstVisibleIndex;
	unsigned int m_uListingSelectedIndex;

	MassStorage *const m_oMS;
};

class ImageMenuItem : public MenuItem
{
public:
	void* operator new(size_t sz) { return Allocate<ImageMenuItem>(); }
	void operator delete(void* p) { Release<ImageMenuItem>(p); }

	ImageMenuItem(PixelNumber r, PixelNumber c, Visibility vis, const char *pFileName);

	void Draw(Lcd7920& lcd, PixelNumber rightMargin, bool highlight, PixelNumber tOffset) override;
	void UpdateWidth(Lcd7920& lcd) override;

private:
	String<MaxFilenameLength> fileName;
};

#endif /* SRC_DISPLAY_MENUITEM_H_ */