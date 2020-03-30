/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/**
 * @file vollist.c
 * @brief List of volumes.
 * @addtogroup ListOfVolumes
 * @{
 */

/*
* Revised by Stefan Pendl, 2010, 2011
* <stefanpe@users.sourceforge.net>
*/

#include "main.h"

WNDPROC OldListWndProc;
HIMAGELIST hImgList;

/* forward declaration */
LRESULT CALLBACK ListWndProc(HWND, UINT, WPARAM, LPARAM);
volume_processing_job * get_first_selected_job(void);
static void InitImageList(void);
static void DestroyImageList(void);

int column_widths_adjusted = 0;

/**
 * @brief Initializes the list of volumes.
 */
void InitVolList(void)
{
    LV_COLUMNW lvc;
    LV_ITEM lvi;
    
    if(WaitForSingleObject(hLangPackEvent,INFINITE) != WAIT_OBJECT_0){
        WgxDbgPrintLastError("InitVolList: wait on hLangPackEvent failed");
        return;
    }
    
    (void)SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,
        (LRESULT)(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT));

    /* create header */
    lvc.mask = LVCF_TEXT;
    lvc.pszText = WgxGetResourceString(i18n_table,L"VOLUME");
    (void)SendMessage(hList,LVM_INSERTCOLUMNW,0,(LRESULT)&lvc);

    lvc.pszText = WgxGetResourceString(i18n_table,L"STATUS");
    (void)SendMessage(hList,LVM_INSERTCOLUMNW,1,(LRESULT)&lvc);

    lvc.mask |= LVCF_FMT;
    lvc.fmt = LVCFMT_RIGHT;
    lvc.pszText = WgxGetResourceString(i18n_table,L"TOTAL");
    (void)SendMessage(hList,LVM_INSERTCOLUMNW,2,(LRESULT)&lvc);

    lvc.pszText = WgxGetResourceString(i18n_table,L"FREE");
    (void)SendMessage(hList,LVM_INSERTCOLUMNW,3,(LRESULT)&lvc);

    lvc.pszText = WgxGetResourceString(i18n_table,L"PERCENT");
    (void)SendMessage(hList,LVM_INSERTCOLUMNW,4,(LRESULT)&lvc);

    OldListWndProc = WgxSafeSubclassWindow(hList,ListWndProc);
    (void)SendMessage(hList,LVM_SETBKCOLOR,0,RGB(255,255,255));
    InitImageList();

    /* force list of volumes to be resized properly */
    lvi.iItem = 0;
    lvi.iSubItem = 1;
    lvi.mask = LVIF_TEXT | LVIF_IMAGE;
    lvi.pszText = "hi";
    lvi.iImage = 0;
    (void)SendMessage(hList,LVM_INSERTITEM,0,(LRESULT)&lvi);
    SetEvent(hLangPackEvent);
}

/**
 * @brief Custom window procedure for the list control.
 */
LRESULT CALLBACK ListWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch(iMsg){
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_KEYDOWN:
    case WM_CHAR:
        /* no actions are allowed while processing volumes */
        if(busy_flag) return 0;
        break;
    case WM_VSCROLL:
        /* why? */
        (void)InvalidateRect(hList,NULL,TRUE);
        (void)UpdateWindow(hList);
        break;
    }
    return WgxSafeCallWndProc(OldListWndProc,hWnd,iMsg,wParam,lParam);
}

/**
 * @brief Adjusts widths of the volume list columns.
 */
static void AdjustVolListColumns(void)
{
    int cw[] = {90,90,110,125,90};
    int total_width = 0;
    int i, width;
    RECT rc;
    
    VolListGetColumnWidths();
    
    /* adjust columns widths */
    if(GetClientRect(hList,&rc))
        width = rc.right - rc.left;
    else
        width = 586;

    if(user_defined_column_widths[0]){
        for(i = 0; i < 5; i++)
            total_width += user_defined_column_widths[i];
        for(i = 0; i < 5; i++){
            (void)SendMessage(hList,LVM_SETCOLUMNWIDTH,i,
                user_defined_column_widths[i] * width / total_width);
        }
    } else {
        for(i = 0; i < 5; i++){
            (void)SendMessage(hList,LVM_SETCOLUMNWIDTH,i,
                cw[i] * width / 505);
        }
    }
    column_widths_adjusted = 1;
}

/**
 * @brief Resizes the list of volumes.
 */
int ResizeVolList(int x, int y, int width, int height, int expand)
{
    int border_height;
    int border_width;
    int header_height = 0;
    int item_height = 0;
    int n_items;
    int new_height;
    HWND hHeader;
    RECT rc;

    /* adjust height of the list */
    border_height = GetSystemMetrics(SM_CYEDGE);
    border_width = GetSystemMetrics(SM_CXEDGE);
    hHeader = (HWND)(LONG_PTR)SendMessage(hList,LVM_GETHEADER,0,0);
    if(hHeader){
        if(SendMessage(hHeader,HDM_GETITEMRECT,0,(LRESULT)&rc))
            header_height = rc.bottom - rc.top;
    }
    rc.top = 1;
    rc.left = LVIR_BOUNDS;
    if(SendMessage(hList,LVM_GETSUBITEMRECT,0,(LRESULT)&rc))
        item_height = rc.bottom - rc.top;
    if(header_height && item_height){
        /* ensure that an integer number of items will be displayed */
        n_items = (height - 2 * border_height - header_height) / item_height;
        new_height = header_height + n_items * item_height + 2 * border_height + 2;
        if(expand && new_height < height){
            n_items ++;
            new_height += item_height;
        }
        height = new_height;
    }
    (void)SetWindowPos(hList,0,x,y,width,height,0);

    AdjustVolListColumns();
    return height;
}

/**
 * @brief Calculates a minimal height of the list.
 */
int GetMinVolListHeight(void)
{
    return DPI(MIN_LIST_HEIGHT);
}

/**
 * @brief Calculates a maximal height of the list.
 */
int GetMaxVolListHeight(void)
{
    RECT rc;
    int h, tb_height, sb_height;
    
    if(!GetClientRect(hWindow,&rc)){
        WgxDbgPrintLastError("GetMaxVolListHeight: cannot get main window dimensions");
        return DPI(VLIST_HEIGHT);
    }
    h = rc.bottom - rc.top;
    
    if(GetWindowRect(hToolbar,&rc))
        tb_height = rc.bottom - rc.top;
    else
        tb_height = 24 + 2 * GetSystemMetrics(SM_CYEDGE);

    if(!GetClientRect(hStatus,&rc)){
        WgxDbgPrintLastError("GetMaxVolListHeight: cannot get status bar dimensions");
        return DPI(VLIST_HEIGHT);
    } else {
        if(!MapWindowPoints(hStatus,hWindow,(LPPOINT)(PRECT)(&rc),(sizeof(RECT)/sizeof(POINT)))){
            WgxDbgPrintLastError("GetMaxVolListHeight: MapWindowPoints failed");
            return DPI(VLIST_HEIGHT);
        } else {            
            sb_height = rc.bottom - rc.top;
        }
    }

    return (h - tb_height - sb_height);
}

/**
 * @brief Frees resources allocated for the list of volumes.
 */
void ReleaseVolList(void)
{
    DestroyImageList();
}

/**
 * @brief Handles notifications from the list of volumes.
 */
void VolListNotifyHandler(LPARAM lParam)
{
    volume_processing_job *job;
    LPNMLISTVIEW lpnm;
    
    lpnm = (LPNMLISTVIEW)lParam;
    if(lpnm->hdr.code == LVN_ITEMCHANGED && lpnm->iItem != (-1)){
        job = get_first_selected_job();
        if(job == NULL){
            /* this may happen when user selects an empty cell */
            job = current_job;
        }
        current_job = job;
        /*HideProgress();*/
        RedrawMap(job,0);
        if(job) UpdateStatusBar(&job->pi);
    }
    /* perform a volume analysis when list item becomes double-clicked */
    if(lpnm->hdr.hwndFrom == hList && lpnm->hdr.code == NM_DBLCLK)
        PostMessage(hWindow,WM_COMMAND,(WPARAM)IDM_ANALYZE,0);
}

/**
 * @brief Updates volume capacity in the list.
 */
static void AddCapacityInformation(int index, volume_info *v)
{
    LV_ITEM lvi;
    char s[32];
    ULONGLONG free, total;
    double d = 0;
    int p;

    lvi.mask = LVIF_TEXT;
    lvi.iItem = index;

    (void)udefrag_bytes_to_hr((ULONGLONG)(v->total_space.QuadPart),2,s,sizeof(s));
    lvi.iSubItem = 2;
    lvi.pszText = s;
    (void)SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);

    (void)udefrag_bytes_to_hr((ULONGLONG)(v->free_space.QuadPart),2,s,sizeof(s));
    lvi.iSubItem = 3;
    lvi.pszText = s;
    (void)SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);
    
    /* conversion to LONGLONG is needed for Win DDK */
    /* so, let's divide both numbers to make safe conversion then */
    total = v->total_space.QuadPart / 2;
    free = v->free_space.QuadPart / 2;
    if(total > 0)
        d = (double)(LONGLONG)free / (double)(LONGLONG)total;
    p = (int)(100 * d);
    (void)sprintf(s,"%u %%",p);
    lvi.iSubItem = 4;
    lvi.pszText = s;
    (void)SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);
}

/**
 * @brief Updates volume processing status in the list.
 */
static void VolListUpdateStatusFieldInternal(int index,volume_processing_job *job)
{
    LV_ITEMW lviw;
    wchar_t *ProcessCaption = L"";
    wchar_t buffer[128], PassString[32] = L"", MoveString[32] = L"", PercentString[32] = L"";

    if(WaitForSingleObject(hLangPackEvent,INFINITE) != WAIT_OBJECT_0){
        WgxDbgPrintLastError("VolListUpdateStatusFieldInternal: wait on hLangPackEvent failed");
        return;
    }
    
    lviw.mask = LVIF_TEXT;
    lviw.iItem = index;
    lviw.iSubItem = 1;
    
    /* each job starts with a volume analysis */
    if(job->pi.current_operation == VOLUME_ANALYSIS && job->job_type != NEVER_EXECUTED_JOB){
        ProcessCaption = WgxGetResourceString(i18n_table,L"STATUS_ANALYSED");
    } else {
        switch(job->job_type){
            case ANALYSIS_JOB:
                ProcessCaption = WgxGetResourceString(i18n_table,L"STATUS_ANALYSED");
                break;
            case DEFRAGMENTATION_JOB:
                ProcessCaption = WgxGetResourceString(i18n_table,L"STATUS_DEFRAGMENTED");
                break;
            case FULL_OPTIMIZATION_JOB:
            case QUICK_OPTIMIZATION_JOB:
            case MFT_OPTIMIZATION_JOB:
                ProcessCaption = WgxGetResourceString(i18n_table,L"STATUS_OPTIMIZED");
                break;
        }
    }
    
    if(job->pi.completion_status < 0){
        lviw.pszText = L"";
    } else {
        if(job->pi.completion_status == 0 || stop_pressed){
            _snwprintf(PercentString,sizeof(PercentString)/sizeof(wchar_t),L"%5.2lf %% ",job->pi.percentage);
            PercentString[sizeof(PercentString)/sizeof(wchar_t) - 1] = 0;
            
            if(job->pi.pass_number > 1){
                _snwprintf(PassString,sizeof(PassString)/sizeof(wchar_t),L", Pass %d",job->pi.pass_number);
                PassString[sizeof(PassString)/sizeof(wchar_t) - 1] = 0;
            }
                
            if(job->pi.current_operation == VOLUME_OPTIMIZATION){
                _snwprintf(MoveString,sizeof(MoveString)/sizeof(wchar_t),L", %I64u moves total",job->pi.total_moves);
                MoveString[sizeof(MoveString)/sizeof(wchar_t) - 1] = 0;
            }
        } else {
            if(job->pi.pass_number > 1){
                _snwprintf(PassString,sizeof(PassString)/sizeof(wchar_t),L", %d passes needed",job->pi.pass_number);
                PassString[sizeof(PassString)/sizeof(wchar_t) - 1] = 0;
            }
        }
        _snwprintf(buffer,sizeof(buffer)/sizeof(wchar_t),L"%ls%ls%ls%ls",PercentString,ProcessCaption,PassString,MoveString);
        buffer[sizeof(buffer)/sizeof(wchar_t) - 1] = 0;
        lviw.pszText = buffer;
    }

    (void)SendMessage(hList,LVM_SETITEMW,0,(LRESULT)&lviw);
    SetEvent(hLangPackEvent);
}

/**
 * @brief Adds a single volume to the list.
 */
static void VolListAddItem(int index, volume_info *v)
{
    volume_processing_job *job;
    LV_ITEMW lvi;
    wchar_t fsname[64];
    wchar_t vname[64 + MAX_PATH + 1];

    _snwprintf(fsname,64,L"%c: [%hs]",v->letter,v->fsname);
    fsname[63] = 0;
    _snwprintf(vname,sizeof(vname)/sizeof(wchar_t),L"%-10ws %ws",fsname,v->label);
    vname[sizeof(vname)/sizeof(wchar_t) - 1] = 0;
    lvi.mask = LVIF_TEXT | LVIF_IMAGE;
    lvi.iItem = index;
    lvi.iSubItem = 0;
    lvi.pszText = vname;
    lvi.iImage = v->is_removable ? 1 : 0;
    (void)SendMessage(hList,LVM_INSERTITEMW,0,(LRESULT)&lvi);

    job = get_job(v->letter);
    VolListUpdateStatusFieldInternal(index,job);
    AddCapacityInformation(index,v);
}

/**
 * @brief UpdateVolList thread routine.
 */
static DWORD WINAPI RescanDrivesThreadProc(LPVOID lpParameter)
{
    volume_processing_job *job;
    volume_info *v;
    LV_ITEM lvi;
    int i;
    
/*  WgxDisableWindows(hWindow,IDC_RESCAN,IDC_ANALYSE,
        IDC_DEFRAGM,IDC_OPTIMIZE,IDC_SHOWFRAGMENTED,0);
    HideProgress();
*/
    /* refill the volume list control */
    (void)SendMessage(hList,LVM_DELETEALLITEMS,0,0);
    v = udefrag_get_vollist(skip_removable);
    if(v){
        for(i = 0; v[i].letter != 0; i++)
            VolListAddItem(i,&v[i]);
        udefrag_release_vollist(v);
    }

    /* select the first item */
    lvi.mask = LVIF_STATE;
    lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
    (void)SendMessage(hList,LVM_SETITEMSTATE,0,(LRESULT)&lvi);
    
    /* scrollbar may appear/disappear after a scan */
    AdjustVolListColumns();
    
    job = get_first_selected_job();
    current_job = job;
    RedrawMap(job,0);
    if(job) UpdateStatusBar(&job->pi);
    
/*  WgxEnableWindows(hWindow,IDC_RESCAN,IDC_ANALYSE,
        IDC_DEFRAGM,IDC_OPTIMIZE,IDC_SHOWFRAGMENTED,0);
*/  return 0;
}

/**
 * @brief Updates the list of volumes.
 */
void UpdateVolList(void)
{
    DWORD id;
    HANDLE h;

    h = create_thread(RescanDrivesThreadProc,NULL,&id);
    if(h == NULL){
        WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,
            "Cannot create thread starting drives rescan!");
    } else {
        CloseHandle(h);
    }
}

/**
 * @brief Saves widths of the volume list columns.
 */
void VolListGetColumnWidths(void)
{
    int i;
    
    if(column_widths_adjusted == 0)
        return; /* we haven't set widths yet */

    for(i = 0; i < 5; i++){
        user_defined_column_widths[i] = \
            (int)(LONG_PTR)SendMessage(hList,LVM_GETCOLUMNWIDTH,i,0);
    }
}

/**
 * @brief Retrieves the first job selected in the list.
 */
volume_processing_job * get_first_selected_job(void)
{
    LRESULT SelectedItem;
    LV_ITEM lvi;
    char buffer[64];
    
    SelectedItem = SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
    if(SelectedItem != -1){
        lvi.iItem = (int)SelectedItem;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_TEXT;
        lvi.pszText = buffer;
        lvi.cchTextMax = 63;
        if(SendMessage(hList,LVM_GETITEM,0,(LRESULT)&lvi)){
            return get_job(buffer[0]);
        }
    }
    return NULL;
}

/**
 * @brief Retrieves an index of the specified job
 * as it is listed in the list of volumes.
 */
int get_job_index(volume_processing_job *job)
{
    LV_ITEM lvi;
    char buffer[64];
    int index = -1;
    int item;

    if(job == NULL)
        return (-1);

    while(1){
        item = (int)SendMessage(hList,LVM_GETNEXTITEM,(WPARAM)index,LVNI_ALL);
        if(item == -1 || item == index) break;
        index = item;
        lvi.iItem = index;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_TEXT;
        lvi.pszText = buffer;
        lvi.cchTextMax = 63;
        if(SendMessage(hList,LVM_GETITEM,0,(LRESULT)&lvi)){
            if(udefrag_tolower(buffer[0]) == udefrag_tolower(job->volume_letter))
                return index;
        }
    }

    return (-1);
}

/**
 * @brief Updates job processing status in the list.
 * @details Decides themselves on which position
 * in list the job locates.
 */
void VolListUpdateStatusField(volume_processing_job *job)
{
    int index;
    
    index = get_job_index(job);
    if(index != -1)
        VolListUpdateStatusFieldInternal(index,job);
}

/**
 * @brief Updates volume capacity fields in the list.
 */
void VolListRefreshItem(volume_processing_job *job)
{
    volume_info v;
    int index;
    
    index = get_job_index(job);
    if(index != -1){
        if(udefrag_get_volume_information(job->volume_letter,&v) >= 0)
            AddCapacityInformation(index,&v);
    }
}

/**
 * @brief Selects all drives in the list.
 */
void SelectAllDrives(void)
{
    LV_ITEM lvi;

    lvi.stateMask = LVIS_SELECTED;
    lvi.state = LVIS_SELECTED;
    (void)SendMessage(hList,LVM_SETITEMSTATE,-1,(LRESULT)&lvi);
}

/**
 * @brief InitVolList helper.
 */
static void InitImageList(void)
{
    hImgList = ImageList_Create(16,16,ILC_COLOR8,2,0);
    if(hImgList == NULL){
        WgxDbgPrintLastError("InitImageList: ImageList_Create failed");
    } else {
        ImageList_AddIcon(hImgList,LoadIcon(hInstance,MAKEINTRESOURCE(IDI_FIXED)));
        ImageList_AddIcon(hImgList,LoadIcon(hInstance,MAKEINTRESOURCE(IDI_REMOVABLE)));
        SendMessage(hList,LVM_SETIMAGELIST,LVSIL_SMALL,(LRESULT)hImgList);
    }
}

/**
 * @brief ReleaseVolList helper.
 */
static void DestroyImageList(void)
{
    if(hImgList)
        ImageList_Destroy(hImgList);
}

/** @} */
