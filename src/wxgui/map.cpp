//////////////////////////////////////////////////////////////////////////
//
//  UltraDefrag - a powerful defragmentation tool for Windows NT.
//  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
//  Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//////////////////////////////////////////////////////////////////////////

/**
 * @file map.cpp
 * @brief Cluster map.
 * @details Even wxBufferedPaintDC
 * is too expensive and causes flicker
 * on map resize, so we're using low
 * level API here.
 * @addtogroup ClusterMap
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// The 'Flicker Free Drawing' article of James Brown
// helped us to reduce GUI flicker on window resize:
// http://www.catch22.net/tuts/flicker

// =======================================================================
//                            Declarations
// =======================================================================

#include "prec.h"
#include "main.h"

COLORREF g_colors[SPACE_STATES] =
{
    RGB(178,175,168),               /* unused map block */
    RGB(255,255,255),               /* free */
    RGB(0,215,32),RGB(4,164,0),     /* system */
    RGB(255,0,0),RGB(128,0,0),      /* fragmented */
    RGB(0,0,255),RGB(0,0,128),      /* unfragmented */
    RGB(255,255,0),RGB(238,221,0),  /* directories */
    RGB(185,185,0),RGB(93,93,0),    /* compressed */
    RGB(211,0,255),RGB(128,0,128),  /* mft zone; mft itself */
};

// =======================================================================
//                            Cluster map
// =======================================================================

ClusterMap::ClusterMap(wxWindow* parent) : wxWindow(parent,wxID_ANY)
{
    HDC hdc = GetDC((HWND)GetHandle());
    m_cacheDC = ::CreateCompatibleDC(hdc);
    if(!m_cacheDC) letrace("cannot create cache dc");
    m_cacheBmp = ::CreateCompatibleBitmap(hdc,
        wxGetDisplaySize().GetWidth(),
        wxGetDisplaySize().GetHeight()
    );
    if(!m_cacheBmp) letrace("cannot create cache bitmap");
    ::SelectObject(m_cacheDC,m_cacheBmp);
    ::SetBkMode(m_cacheDC,TRANSPARENT);
    ::ReleaseDC((HWND)GetHandle(),hdc);

    for(int i = 0; i < SPACE_STATES; i++)
        m_brushes[i] = ::CreateSolidBrush(g_colors[i]);

    m_width = m_height = 0;
}

ClusterMap::~ClusterMap()
{
    ::DeleteDC(m_cacheDC);
    ::DeleteObject(m_cacheBmp);
    for(int i = 0; i < SPACE_STATES; i++)
        ::DeleteObject(m_brushes[i]);
}

// =======================================================================
//                           Event handlers
// =======================================================================

BEGIN_EVENT_TABLE(ClusterMap, wxWindow)
    EVT_ERASE_BACKGROUND(ClusterMap::OnEraseBackground)
    EVT_PAINT(ClusterMap::OnPaint)
END_EVENT_TABLE()

void ClusterMap::OnEraseBackground(wxEraseEvent& event)
{
    int width, height; GetClientSize(&width,&height);
    if(width > m_width || height > m_height){
        // expand free space to reduce flicker
        HDC hdc = GetDC((HWND)GetHandle());

        char free_r = (char)g_mainFrame->CheckOption(wxT("UD_FREE_COLOR_R"));
        char free_g = (char)g_mainFrame->CheckOption(wxT("UD_FREE_COLOR_G"));
        char free_b = (char)g_mainFrame->CheckOption(wxT("UD_FREE_COLOR_B"));
        HBRUSH brush = ::CreateSolidBrush(RGB(free_r,free_g,free_b));

        RECT rc; rc.left = m_width; rc.top = 0;
        rc.right = width; rc.bottom = height;
        ::FillRect(hdc,&rc,brush);

        rc.left = 0; rc.top = m_height;
        rc.right = width; rc.bottom = height;
        ::FillRect(hdc,&rc,brush);

        ::DeleteObject(brush);

        ::ReleaseDC((HWND)GetHandle(),hdc);
    }
    m_width = width; m_height = height;
}

/**
 * @brief Scales cluster map
 * of the current job to fit
 * inside of the map control.
 */
char *ClusterMap::ScaleMap(int scaled_size)
{
    JobsCacheEntry *currentJob = g_mainFrame->m_currentJob;
    int map_size = currentJob->pi.cluster_map_size;

    // dtrace("map size = %u, scaled size = %u",map_size,scaled_size);

    if(scaled_size == map_size)
        return NULL; // no need to scale

    char *scaledMap = new char[scaled_size];

    int ratio = scaled_size / map_size;
    int used_cells = 0;
    if(ratio){
        // scale up
        for(int i = 0; i < map_size; i++){
            for(int j = 0; j < ratio; j++){
                scaledMap[used_cells] = currentJob->clusterMap[i];
                used_cells ++;
            }
        }
    } else {
        // scale down
        ratio = map_size / scaled_size;
        if(ratio * scaled_size != map_size)
            ratio ++; /* round up */

        used_cells = map_size / ratio;
        for(int i = 0; i < used_cells; i++){
            int states[SPACE_STATES];
            memset(states,0,sizeof(states));
            bool mft_detected = false;

            int sequence_length = (i < used_cells - 1) ? \
                ratio : map_size - i * ratio;

            for(int j = 0; j < sequence_length; j++){
                int index = (int)currentJob->clusterMap[i * ratio + j];
                if(index >= 0 && index < SPACE_STATES) states[index] ++;
                if(index == MFT_SPACE) mft_detected = true;
            }

            if(mft_detected){
                scaledMap[i] = MFT_SPACE;
            } else {
                // draw cell in dominating color
                int maximum = states[0]; scaledMap[i] = 0;
                for(int j = 1; j < SPACE_STATES; j++){
                    if(states[j] >= maximum){
                        maximum = states[j];
                        scaledMap[i] = (char)j;
                    }
                }
            }
        }
    }

    // mark unused cells
    for(int i = used_cells; i < scaled_size; i++)
        scaledMap[i] = UNUSED_MAP_SPACE;

    return scaledMap;
}

void ClusterMap::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    JobsCacheEntry *currentJob;
    int width, height; GetClientSize(&width,&height);

    int block_size = g_mainFrame->CheckOption(wxT("UD_MAP_BLOCK_SIZE"));
    int line_width = g_mainFrame->CheckOption(wxT("UD_GRID_LINE_WIDTH"));

    int cell_size = block_size + line_width;
    int blocks_per_line = cell_size ? (width - line_width) / cell_size : 0;
    int lines = cell_size ? (height - line_width) / cell_size : 0;

    // fill map by the free color
    char free_r = (char)g_mainFrame->CheckOption(wxT("UD_FREE_COLOR_R"));
    char free_g = (char)g_mainFrame->CheckOption(wxT("UD_FREE_COLOR_G"));
    char free_b = (char)g_mainFrame->CheckOption(wxT("UD_FREE_COLOR_B"));
    HBRUSH brush = ::CreateSolidBrush(RGB(free_r,free_g,free_b));
    RECT rc; rc.left = rc.top = 0; rc.right = width; rc.bottom = height;
    ::FillRect(m_cacheDC,&rc,brush); ::DeleteObject(brush);
    if(!blocks_per_line || !lines) goto draw;

    // draw grid
    if(line_width){
        char grid_r = (char)g_mainFrame->CheckOption(wxT("UD_GRID_COLOR_R"));
        char grid_g = (char)g_mainFrame->CheckOption(wxT("UD_GRID_COLOR_G"));
        char grid_b = (char)g_mainFrame->CheckOption(wxT("UD_GRID_COLOR_B"));
        brush = ::CreateSolidBrush(RGB(grid_r,grid_g,grid_b));
        for(int i = 0; i < blocks_per_line + 1; i++){
            RECT rc; rc.left = cell_size * i; rc.top = 0;
            rc.right = rc.left + line_width;
            rc.bottom = cell_size * lines + line_width;
            ::FillRect(m_cacheDC,&rc,brush);
        }
        for(int i = 0; i < lines + 1; i++){
            RECT rc; rc.left = 0; rc.top = cell_size * i;
            rc.right = cell_size * blocks_per_line + line_width;
            rc.bottom = rc.top + line_width;
            ::FillRect(m_cacheDC,&rc,brush);
        }
        ::DeleteObject(brush);
    }

    // draw squares
    currentJob = g_mainFrame->m_currentJob;
    if(currentJob){
        if(currentJob->pi.cluster_map_size){
            int scaled_size = blocks_per_line * lines;
            char *scaledMap = ScaleMap(scaled_size);

            // draw either normal or scaled map
            char *map = scaledMap ? scaledMap : currentJob->clusterMap;
            for(int i = 0; i < lines; i++){
                for(int j = 0; j < blocks_per_line; j++){
                    RECT rc;
                    rc.top = cell_size * i + line_width;
                    rc.left = cell_size * j + line_width;
                    rc.right = rc.left + block_size;
                    rc.bottom = rc.top + block_size;
                    int index = (int)map[i * blocks_per_line + j];
                    if(index != FREE_SPACE){
                        ::FillRect(m_cacheDC,&rc,m_brushes[index]);
                    }
                }
            }

            delete [] scaledMap;
        }
    }

draw:
    // draw map on the screen
    PAINTSTRUCT ps;
    HDC hdc = ::BeginPaint((HWND)GetHandle(),&ps);
    ::BitBlt(hdc,0,0,width,height,m_cacheDC,0,0,SRCCOPY);
    ::EndPaint((HWND)GetHandle(),&ps);
}

void MainFrame::RedrawMap(wxCommandEvent& WXUNUSED(event))
{
    m_cMap->Refresh();
}

/** @} */
