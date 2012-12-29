/*
 Copyright (C) 2012 by Leo Franchi <lfranchi@kde.org>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include "qtoolbartabdialog.h"

#include "qocoa_mac.h"

#import <Cocoa/Cocoa.h>

#include <QCoreApplication>
#include <QIcon>
#include <QMap>
#include <QUuid>
#include <QMacNativeWidget>
#include <QPointer>

struct ItemData {
    QPixmap icon;
    QString text, tooltip;
    QMacNativeWidget* nativeWidget;
    QWidget* page;
};


namespace {

static const int TOOLBAR_ITEM_WIDTH = 32;

CGFloat ToolbarHeightForWindow(NSWindow *window)
{
    CGFloat toolbarHeight = 0.0f;

    NSToolbar *toolbar = toolbar = [window toolbar];

    if(toolbar && [toolbar isVisible])
    {
        NSRect windowFrame = [NSWindow contentRectForFrameRect:[window frame]
                                              styleMask:[window styleMask]];
        toolbarHeight = NSHeight(windowFrame) - NSHeight([[window contentView] frame]);
    }

    return toolbarHeight;
}

};

@interface ToolbarDelegate : NSObject<NSToolbarDelegate, NSWindowDelegate>
{
    QToolbarTabDialogPrivate *pimpl;
}
// Internal
-(void)setPrivate:(QToolbarTabDialogPrivate*)withPimpl;

// NSToolbarItem action
-(void)changePanes:(id)sender;

// NSToolbarDelegate
-(NSToolbarItem *) toolbar: (NSToolbar *)toolbar itemForItemIdentifier: (NSString *) itemIdent willBeInsertedIntoToolbar:(BOOL) willBeInserted;
-(NSArray*) toolbarAllowedItemIdentifiers: (NSToolbar *) toolbar;
-(NSArray*) toolbarDefaultItemIdentifiers: (NSToolbar *) toolbar;
-(NSArray*) toolbarSelectableItemIdentifiers: (NSToolbar*)toolbar;

// NSWindowDelegate
-(NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize;
-(void)windowWillClose:(NSNotification *)notification;
@end

class QToolbarTabDialogPrivate {
public:
    QToolbarTabDialogPrivate(QToolbarTabDialog* dialog) : q(dialog),
                                                          currentPane(NULL),
                                                          minimumWidth(0)
    {
    }

    ~QToolbarTabDialogPrivate() {
        // unset the delegate and toolbar from the window and manually release them
        // otherwise, for some reason the old delegate is laying around when we
        // create a new NSWindow
        [[prefsWindow toolbar] setDelegate:NULL];
        [prefsWindow setToolbar:NULL];
        [prefsWindow release];
        [toolBar release];
        [toolBarDelegate release];
    }

    void calculateSize() {
        NSRect windowFrame = [prefsWindow frame];

        while ([[toolBar visibleItems] count] < [[toolBar items] count]) {
            //Each toolbar item is 32x32; we expand by one toolbar item width repeatedly until they all fit
            windowFrame.origin.x -= TOOLBAR_ITEM_WIDTH / 2;
            windowFrame.size.width += TOOLBAR_ITEM_WIDTH / 2;
            
            [prefsWindow setFrame:windowFrame display:NO];
            [prefsWindow setMinSize: windowFrame.size];
        }
        minimumWidth = windowFrame.size.width;
    }

    void showPaneWithIdentifier(NSString* ident) {
        NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

        const QString identStr = toQString(ident);
        Q_ASSERT(items.contains(identStr));
        if (!items.contains(identStr))
            return;

        QWidget* newWidget = items[identStr].nativeWidget;
        Q_ASSERT(newWidget);
        if (!newWidget)
            return;

        QWidget* newPage = items[identStr].page;
        Q_ASSERT(newPage);
        if (!newPage)
            return;
        
        const NSRect oldFrame = [[prefsWindow contentView] frame];

        // Clear first responder on window and set a temporary NSView on the window
        // while we change the widget out underneath
        [prefsWindow makeFirstResponder:nil];

        NSView *tempView = [[NSView alloc] initWithFrame:[[prefsWindow contentView] frame]];
        [prefsWindow setContentView:tempView];
        [tempView release];

        QSize sizeToUse = newPage->sizeHint().isNull() ? newPage->size() : newPage->sizeHint();
        sizeToUse.setWidth(qMax(sizeToUse.width(), newPage->minimumWidth()));
        sizeToUse.setHeight(qMax(sizeToUse.height(), newPage->minimumHeight()));

        static const int spacing = 4;

        [prefsWindow setMinSize:NSMakeSize(sizeToUse.width(), sizeToUse.height())];

        // Make room for the new view
        NSRect newFrame = [prefsWindow frame];
        newFrame.size.height = sizeToUse.height() + ([prefsWindow frame].size.height - [[prefsWindow contentView] frame].size.height) + spacing;
        newFrame.size.width = sizeToUse.width() + spacing;

        // Don't resize the width---only the height, so use the maximum width for any page
        // or the same width as before, if the user already resized it to be larger.
        newFrame.size.width < minimumWidth ? newFrame.size.width =  minimumWidth
                                           : newFrame.size.width = qMax(newFrame.size.width, oldFrame.size.width);

        // Preserve upper left point of window during resize.
        newFrame.origin.y += ([[prefsWindow contentView] frame].size.height - sizeToUse.height()) - spacing;

        [prefsWindow setFrame:newFrame display:YES animate:YES];

        [prefsWindow setContentView: [panes objectForKey:ident]];
        currentPane = ident;

        // Resize the Qt widget immediately as well
        resizeCurrentPageToSize([[prefsWindow contentView] frame].size);

        NSSize minSize = [prefsWindow frame].size;
        minSize.height -= ToolbarHeightForWindow(prefsWindow);

        [prefsWindow setMinSize:minSize];

        BOOL canResize = YES;
        NSSize maxSize = NSMakeSize(FLT_MAX, FLT_MAX);

        if (newPage->sizePolicy().horizontalPolicy() == QSizePolicy::Fixed) {
            canResize = NO;
            maxSize.width = sizeToUse.width();
        }
        if (newPage->sizePolicy().verticalPolicy() == QSizePolicy::Fixed) {
            canResize = NO;
            maxSize.height = sizeToUse.height();
        }


        [prefsWindow setMaxSize:maxSize];
        [prefsWindow setShowsResizeIndicator:canResize];

        [prefsWindow setTitle:ident];
        [prefsWindow makeFirstResponder:[panes objectForKey:ident]];

        [pool drain];
    }

    void resizeCurrentPageToSize(NSSize frameSize) {
        const QString curPane = toQString(currentPane);
        if (items.contains(curPane) && items[curPane].nativeWidget) {
            items[curPane].nativeWidget->resize(frameSize.width, frameSize.height);
        }
    }

    void emitAccepted() {
        if (q.isNull())
            return;

        q.data()->accepted();
    }

    QWeakPointer<QToolbarTabDialog> q;

    NSWindow* prefsWindow;
    ToolbarDelegate *toolBarDelegate;
    QMap<QString, ItemData> items;

    NSMutableDictionary *panes;
    NSToolbar *toolBar;
    NSString* currentPane;

    int minimumWidth;
};


@implementation ToolbarDelegate

-(id) init {
    if( self = [super init] )
    {
        pimpl = nil;
    }

    return self;
}

-(void) setPrivate:(QToolbarTabDialogPrivate *)withPimpl
{
    pimpl = withPimpl;
}

-(void)changePanes:(id)sender
{
    Q_UNUSED(sender);
    if (!pimpl)
        return;

    pimpl->showPaneWithIdentifier([pimpl->toolBar selectedItemIdentifier]);
}

-(NSToolbarItem *) toolbar: (NSToolbar *)toolbar itemForItemIdentifier: (NSString *) itemIdent willBeInsertedIntoToolbar:(BOOL) willBeInserted
{
    Q_UNUSED(toolbar);
    Q_UNUSED(willBeInserted);
    if (!pimpl)
        return nil;

    NSToolbarItem   *toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdent] autorelease];
    const QString identQStr = toQString(itemIdent);
    if (pimpl->items.contains(identQStr))
    {
        const ItemData& data = pimpl->items[identQStr];
        NSString* label = fromQString(data.text);

        [toolbarItem setLabel:label];
        [toolbarItem setPaletteLabel:label];

        [toolbarItem setToolTip:fromQString(data.tooltip)];
        [toolbarItem setImage:fromQPixmap(data.icon)];

        [toolbarItem setTarget: self];
        [toolbarItem setAction: @selector(changePanes:)];

    } else {
        toolbarItem = nil;
    }

    return toolbarItem;
}

-(NSArray*) toolbarAllowedItemIdentifiers: (NSToolbar *) toolbar
{
    Q_UNUSED(toolbar);
    if (!pimpl)
        return [NSArray array];

    NSMutableArray* allowedItems = [[[NSMutableArray alloc] init] autorelease];

    Q_FOREACH( const QString& identQStr, pimpl->items.keys())
        [allowedItems addObject:fromQString(identQStr)];

    [allowedItems addObjectsFromArray:[NSArray arrayWithObjects:NSToolbarSeparatorItemIdentifier,
                                        NSToolbarSpaceItemIdentifier, NSToolbarFlexibleSpaceItemIdentifier,
                                        NSToolbarCustomizeToolbarItemIdentifier, nil] ];

    return allowedItems;
}


-(NSArray*) toolbarDefaultItemIdentifiers: (NSToolbar *) toolbar
{
    Q_UNUSED(toolbar);
    if (!pimpl)
        return [NSArray array];

    return [[[NSMutableArray alloc] initWithArray:[pimpl->panes allKeys]] autorelease];

}


-(NSArray*) toolbarSelectableItemIdentifiers: (NSToolbar*)toolbar
{
    Q_UNUSED(toolbar);
    if (!pimpl)
        return [NSArray array];

    return [[[NSMutableArray alloc] initWithArray:[pimpl->panes allKeys]] autorelease];
}

-(NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{
    if (!pimpl)
        return frameSize;

    pimpl->resizeCurrentPageToSize([[sender contentView] frame].size);

    return frameSize;
}

-(void)windowWillClose:(NSNotification *)notification
{
    Q_UNUSED(notification);

    pimpl->emitAccepted();
}
@end

QToolbarTabDialog::QToolbarTabDialog() :
    QObject(0),
    pimpl(new QToolbarTabDialogPrivate(this))
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    pimpl->panes = [[NSMutableDictionary alloc] init];
    
    static const int defaultWidth = 350;
    static const int defaultHeight = 200;
    pimpl->prefsWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, defaultWidth, defaultHeight)
                                           styleMask:NSClosableWindowMask | NSResizableWindowMask | NSTitledWindowMask
                                           backing:NSBackingStoreBuffered
                                           defer:NO];

    [pimpl->prefsWindow setReleasedWhenClosed:NO];
    [pimpl->prefsWindow setTitle:@"Preferences"];
    
    // identifier is some app-unique string, since all toolbars in an app share state. make this unique to this app's preferences window
    pimpl->toolBar = [[NSToolbar alloc] initWithIdentifier:[NSString stringWithFormat:@"%@.prefspanel.toolbar", fromQString(QCoreApplication::instance()->applicationName())]];
    [pimpl->toolBar setAllowsUserCustomization: NO];
    [pimpl->toolBar setAutosavesConfiguration: NO];
    [pimpl->toolBar setDisplayMode: NSToolbarDisplayModeIconAndLabel];

    pimpl->toolBarDelegate = [[ToolbarDelegate alloc] init];
    [pimpl->toolBarDelegate setPrivate:pimpl.data()];

    [pimpl->prefsWindow setDelegate:pimpl->toolBarDelegate];
    [pimpl->toolBar setDelegate:pimpl->toolBarDelegate];

    [pimpl->prefsWindow setToolbar:pimpl->toolBar];

    [pool drain];
}

QToolbarTabDialog::~QToolbarTabDialog()
{
}

void QToolbarTabDialog::addTab(QWidget* page, const QPixmap& icon, const QString& label, const QString& tooltip)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSString* identifier = fromQString(label);

    QMacNativeWidget* nativeWidget = new QMacNativeWidget;
    nativeWidget->move(0, 0);
    nativeWidget->setPalette(page->palette());
    nativeWidget->setAutoFillBackground(true);

    QVBoxLayout* l = new QVBoxLayout;
    l->setContentsMargins(2, 2, 2, 2);
    l->setSpacing(0);
    page->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    l->addWidget(page);
    nativeWidget->setLayout(l);

    NSView *nativeView = reinterpret_cast<NSView*>(nativeWidget->winId());
    [nativeView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [nativeView setAutoresizesSubviews:YES];

    pimpl->minimumWidth = qMax(pimpl->minimumWidth, page->sizeHint().width());
    
    nativeWidget->show();

    ItemData data;
    data.icon = icon;
    data.text = label;
    data.tooltip = tooltip;
    data.nativeWidget = nativeWidget;
    data.page = page;
    pimpl->items.insert(label, data);

    [pimpl->panes setObject:nativeView forKey:identifier];

    pimpl->showPaneWithIdentifier(identifier);

    [pimpl->toolBar insertItemWithItemIdentifier:identifier atIndex:[[pimpl->toolBar items] count]];
    [pimpl->toolBar setSelectedItemIdentifier:identifier];
    [[pimpl->prefsWindow standardWindowButton:NSWindowZoomButton] setEnabled:NO];

    pimpl->calculateSize();
    [pool drain];
}


void QToolbarTabDialog::setCurrentIndex(int index)
{
    Q_ASSERT(pimpl);
    if (!pimpl)
        return;

    [pimpl->toolBar setSelectedItemIdentifier:[[[pimpl->toolBar items] objectAtIndex:index] itemIdentifier]];
    pimpl->showPaneWithIdentifier([[[pimpl->toolBar items] objectAtIndex:index] itemIdentifier]);
}

void QToolbarTabDialog::show()
{
    Q_ASSERT(pimpl);
    if (!pimpl)
        return;

    [pimpl->prefsWindow center];
    [pimpl->prefsWindow makeKeyAndOrderFront:nil];
}

void QToolbarTabDialog::hide()
{
    Q_ASSERT(pimpl);
    if (!pimpl)
        return;

    [pimpl->prefsWindow close];
    emit accepted();
}

#include "moc_qtoolbartabdialog.cpp"
