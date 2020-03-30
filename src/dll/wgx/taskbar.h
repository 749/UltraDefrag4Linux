/*
* taskbar.h - ITaskbarList3 interface definition.
* Source: WDK7 headers.
*/

#ifndef _TASKBAR_H_
#define _TASKBAR_H_

#include <commctrl.h>

#define CLSID_TaskbarList {0x56FDF344, 0xFD6D, 0x11d0, {0x95, 0x8A, 0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90}}
#define IID_ITaskbarList3 {0xEA1AFB91, 0x9E28, 0x4B86, {0x90, 0xE9, 0x9E, 0x9F, 0x8A, 0x5E, 0xEF, 0xAF}}

#define LPTHUMBBUTTON LPVOID

/* interface definition */
typedef struct _ITaskbarList3 {
    struct _ITaskbarList3Vtbl *lpVtbl;
} ITaskbarList3;

/* v-table of the interface */
typedef struct _ITaskbarList3Vtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        ITaskbarList3 * This,REFIID riid,void **ppvObject);
    
    ULONG (STDMETHODCALLTYPE *AddRef)(ITaskbarList3 * This);
    
    ULONG (STDMETHODCALLTYPE *Release)(ITaskbarList3 * This);
    
    HRESULT (STDMETHODCALLTYPE *HrInit)(ITaskbarList3 * This);
    
    HRESULT (STDMETHODCALLTYPE *AddTab)(ITaskbarList3 * This,HWND hwnd);
    
    HRESULT (STDMETHODCALLTYPE *DeleteTab)(ITaskbarList3 * This,HWND hwnd);
    
    HRESULT (STDMETHODCALLTYPE *ActivateTab)(ITaskbarList3 * This,HWND hwnd);
    
    HRESULT (STDMETHODCALLTYPE *SetActiveAlt)(ITaskbarList3 * This,HWND hwnd);
    
    HRESULT (STDMETHODCALLTYPE *MarkFullscreenWindow)(
        ITaskbarList3 * This,HWND hwnd,BOOL fFullscreen);
    
    HRESULT (STDMETHODCALLTYPE *SetProgressValue)(
        ITaskbarList3 * This,HWND hwnd,
        ULONGLONG ullCompleted,ULONGLONG ullTotal);
    
    HRESULT (STDMETHODCALLTYPE *SetProgressState)(
        ITaskbarList3 * This,HWND hwnd,TBPFLAG tbpFlags);
    
    HRESULT (STDMETHODCALLTYPE *RegisterTab)( 
        ITaskbarList3 * This,HWND hwndTab,HWND hwndMDI);
    
    HRESULT (STDMETHODCALLTYPE *UnregisterTab)( 
        ITaskbarList3 * This,HWND hwndTab);
    
    HRESULT (STDMETHODCALLTYPE *SetTabOrder)( 
        ITaskbarList3 * This,HWND hwndTab,
        HWND hwndInsertBefore);
    
    HRESULT (STDMETHODCALLTYPE *SetTabActive)( 
        ITaskbarList3 * This,HWND hwndTab,
        HWND hwndMDI,DWORD dwReserved);
    
    HRESULT (STDMETHODCALLTYPE *ThumbBarAddButtons)( 
        ITaskbarList3 * This,HWND hwnd,
        UINT cButtons,LPTHUMBBUTTON pButton);
    
    HRESULT (STDMETHODCALLTYPE *ThumbBarUpdateButtons)( 
        ITaskbarList3 * This,HWND hwnd,
        UINT cButtons,LPTHUMBBUTTON pButton);
    
    HRESULT (STDMETHODCALLTYPE *ThumbBarSetImageList)( 
        ITaskbarList3 * This,HWND hwnd,HIMAGELIST himl);
    
    HRESULT (STDMETHODCALLTYPE *SetOverlayIcon)( 
        ITaskbarList3 * This,HWND hwnd,
        HICON hIcon,LPCWSTR pszDescription);
    
    HRESULT (STDMETHODCALLTYPE *SetThumbnailTooltip)( 
        ITaskbarList3 * This,HWND hwnd,LPCWSTR pszTip);
    
    HRESULT (STDMETHODCALLTYPE *SetThumbnailClip)( 
        ITaskbarList3 * This,HWND hwnd,RECT *prcClip);
} ITaskbarList3Vtbl;

/* macro definitions for easy interface access */
#define ITaskbarList3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ITaskbarList3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ITaskbarList3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ITaskbarList3_HrInit(This)	\
    ( (This)->lpVtbl -> HrInit(This) ) 

#define ITaskbarList3_AddTab(This,hwnd)	\
    ( (This)->lpVtbl -> AddTab(This,hwnd) ) 

#define ITaskbarList3_DeleteTab(This,hwnd)	\
    ( (This)->lpVtbl -> DeleteTab(This,hwnd) ) 

#define ITaskbarList3_ActivateTab(This,hwnd)	\
    ( (This)->lpVtbl -> ActivateTab(This,hwnd) ) 

#define ITaskbarList3_SetActiveAlt(This,hwnd)	\
    ( (This)->lpVtbl -> SetActiveAlt(This,hwnd) ) 


#define ITaskbarList3_MarkFullscreenWindow(This,hwnd,fFullscreen)	\
    ( (This)->lpVtbl -> MarkFullscreenWindow(This,hwnd,fFullscreen) ) 


#define ITaskbarList3_SetProgressValue(This,hwnd,ullCompleted,ullTotal)	\
    ( (This)->lpVtbl -> SetProgressValue(This,hwnd,ullCompleted,ullTotal) ) 

#define ITaskbarList3_SetProgressState(This,hwnd,tbpFlags)	\
    ( (This)->lpVtbl -> SetProgressState(This,hwnd,tbpFlags) ) 

#define ITaskbarList3_RegisterTab(This,hwndTab,hwndMDI)	\
    ( (This)->lpVtbl -> RegisterTab(This,hwndTab,hwndMDI) ) 

#define ITaskbarList3_UnregisterTab(This,hwndTab)	\
    ( (This)->lpVtbl -> UnregisterTab(This,hwndTab) ) 

#define ITaskbarList3_SetTabOrder(This,hwndTab,hwndInsertBefore)	\
    ( (This)->lpVtbl -> SetTabOrder(This,hwndTab,hwndInsertBefore) ) 

#define ITaskbarList3_SetTabActive(This,hwndTab,hwndMDI,dwReserved)	\
    ( (This)->lpVtbl -> SetTabActive(This,hwndTab,hwndMDI,dwReserved) ) 

#define ITaskbarList3_ThumbBarAddButtons(This,hwnd,cButtons,pButton)	\
    ( (This)->lpVtbl -> ThumbBarAddButtons(This,hwnd,cButtons,pButton) ) 

#define ITaskbarList3_ThumbBarUpdateButtons(This,hwnd,cButtons,pButton)	\
    ( (This)->lpVtbl -> ThumbBarUpdateButtons(This,hwnd,cButtons,pButton) ) 

#define ITaskbarList3_ThumbBarSetImageList(This,hwnd,himl)	\
    ( (This)->lpVtbl -> ThumbBarSetImageList(This,hwnd,himl) ) 

#define ITaskbarList3_SetOverlayIcon(This,hwnd,hIcon,pszDescription)	\
    ( (This)->lpVtbl -> SetOverlayIcon(This,hwnd,hIcon,pszDescription) ) 

#define ITaskbarList3_SetThumbnailTooltip(This,hwnd,pszTip)	\
    ( (This)->lpVtbl -> SetThumbnailTooltip(This,hwnd,pszTip) ) 

#define ITaskbarList3_SetThumbnailClip(This,hwnd,prcClip)	\
    ( (This)->lpVtbl -> SetThumbnailClip(This,hwnd,prcClip) ) 

#endif /* _TASKBAR_H_ */
