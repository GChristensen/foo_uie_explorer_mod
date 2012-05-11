#pragma once

class MemDC
{
public:
	MemDC(HWND hWnd) //like CPaintDC
		: m_hDC(0), m_hOldDC(0), m_hBitmap(0), m_hOldBitmap(0), m_bFlushed(false), m_hWnd(hWnd)
	{
		ASSERT(hWnd);
		BeginPaint(m_hWnd, &m_ps);
		Init(m_hOldDC = m_ps.hdc, &m_ps.rcPaint);
	}

	MemDC(HDC hDC, LPRECT pRect)
		: m_hDC(0), m_hOldDC(hDC), m_hBitmap(0), m_hOldBitmap(0), m_bFlushed(false), m_hWnd(0)
	{
		Init(hDC, pRect);
	}

	virtual ~MemDC()
	{
		Flush();
	}

	void Flush()
	{
		if (!m_bFlushed) {
			m_bFlushed = true;
			//Swap back the original bitmap.
			if (m_hBitmap) {
				// Copy the offscreen bitmap onto the screen.
				BitBlt(m_hOldDC, m_rect.left, m_rect.top, 
					m_rect.right - m_rect.left, m_rect.bottom - m_rect.top,
					m_hDC, m_rect.left, m_rect.top, SRCCOPY);

				SelectObject(m_hDC, m_hOldBitmap);
				DeleteObject((HGDIOBJ) m_hBitmap);
			}
			if (m_hDC && m_hDC != m_hOldDC)
				DeleteDC(m_hDC);
			if (m_hWnd)
				EndPaint(m_hWnd, &m_ps);
		}
	}

	operator HDC() const
	{
		return m_hDC;
	}

	HDC m_hDC;
private:
	void Init(HDC hDC, LPRECT pRect)
	{
		ASSERT(hDC);

		// Get the rectangle to draw
		if (!pRect)
			GetClipBox(hDC, &m_rect);
		else
			m_rect = *pRect;
		
		// Create a Memory DC
		m_hDC = CreateCompatibleDC(hDC);
		ASSERT(m_hDC);
		LPtoDP(hDC, (LPPOINT) &m_rect, 2);

		if (m_hBitmap = CreateCompatibleBitmap(hDC, m_rect.right - m_rect.left, m_rect.bottom - m_rect.top)) {
			m_hOldBitmap = (HBITMAP) SelectObject(m_hDC, m_hBitmap);

			SetMapMode(m_hDC, GetMapMode(hDC));

			SIZE size;

			GetWindowExtEx(hDC, &size);
			SetWindowExtEx(m_hDC, size.cx, size.cy, NULL);

			GetViewportExtEx(hDC, &size);
			SetViewportExtEx(m_hDC, size.cx, size.cy, NULL);
		}
		else {
			ASSERT(0);
#ifdef ATLTRACE
			ATLTRACE(LastErrorMsg());
#endif
			DeleteDC(m_hDC);
			m_hDC = hDC;
		}

		DPtoLP(hDC, (LPPOINT) &m_rect, 2);

		SetWindowOrgEx(m_hDC, m_rect.left, m_rect.top, NULL);

		// Fill background
		COLORREF clr = GetBkColor(hDC);
		SetBkColor(m_hDC, clr);
		ExtTextOut(m_hDC, 0, 0, ETO_OPAQUE, &m_rect, NULL, 0, NULL);
	}

	HDC m_hOldDC;
	HBITMAP m_hBitmap;
	HBITMAP m_hOldBitmap;
	bool m_bFlushed;
	RECT m_rect;
	HWND m_hWnd;
	PAINTSTRUCT m_ps;
};
