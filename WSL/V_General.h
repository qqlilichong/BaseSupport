
#pragma once

#include "WSL_Win.h"

//////////////////////////////////////////////////////////////////////////

typedef enum { IRMODE_4B3, IRMODE_16B9, IRMODE_FIT } VMODE_RENDER ;

inline VMODE_RENDER AdjustImageRect( _In_ VMODE_RENDER Scale, _In_ RECT rcRender, _Out_ RECT& rcOut )
{
	int cx = 0, cy = 0, x = 0 , y = 0 , n = 1 ;
	int	nRectWidth =  rcRender.right - rcRender.left ;	
	int	nRectHeight = rcRender.bottom - rcRender.top ;
	
	switch(Scale)
	{
	case IRMODE_4B3:
		{
			if (nRectHeight*4 == nRectWidth*3)
			{
				x = 0;
				y = 0;
				cx = nRectWidth;
				cy = nRectHeight;
				break ;
			}

			if (nRectWidth*3 >= nRectHeight*4)
			{
				cy = nRectHeight ;
				cx = nRectHeight/3*4 ;
				y = 0 ;
				x = (nRectWidth - cx)/2 ;
			}

			else 
			{
				cx = nRectWidth;
				cy = nRectWidth/4*3;
				x = 0;
				y = (nRectHeight - cy)/2;
			}
		}
		break;
	case IRMODE_16B9:
		{
			if ((16>nRectWidth)||(9>nRectHeight))
			{
				break ;
			}

			if (nRectHeight*16 == nRectWidth*9)
			{
				x = 0;
				y = 0;
				cx = nRectWidth;
				cy = nRectHeight;
				break ;
			}

			if (nRectWidth*9 >= nRectHeight*16 )
			{
				cy = nRectHeight ;
				cx = nRectHeight/9*16 ;
				y = 0 ;
				x = (nRectWidth - cx)/2 ;
			}

			else
			{
				cx = nRectWidth ;
				cy = nRectWidth/16*9 ;
				x = 0 ;
				y = (nRectHeight - cy)/2 ;
			}
		}
		break;
	case IRMODE_FIT:
		{
			x = 0;
			y = 0;
			cx = nRectWidth;
			cy = nRectHeight;
		}
		break;
	default:
		break;
	}

	rcOut.left = x + rcRender.left ;
	rcOut.top = y + rcRender.top ;
	rcOut.right = cx + rcOut.left ;
	rcOut.bottom = cy + rcOut.top ;

	if ((rcRender.bottom - rcOut.bottom) != (rcOut.top - rcRender.top))
	{
		if (((rcRender.bottom - rcOut.bottom) - (rcOut.top - rcRender.top)) == 1)
		{
			rcOut.bottom++ ;
		}

		else
		{
			rcOut.top-- ;
		}
	}
	if ((rcRender.right - rcOut.right) != (rcOut.left - rcRender.left))
	{
		if (((rcRender.right - rcOut.right) - (rcOut.left - rcRender.left)) == 1)
		{
			rcOut.right++ ;
		}

		else
		{
			rcOut.left-- ;
		}
	}
	return Scale ;
}

inline VMODE_RENDER AdjustImageRect( _In_ int nWPix, _In_ int nPix, _In_ RECT rcRender, _Out_ RECT& rcOut )
{
	VMODE_RENDER ret = IRMODE_FIT ;
	
	if ( nWPix == 0 || nPix == 0 )
	{
		ret = IRMODE_FIT ;
	}
	
	else if ( nWPix * 3 == nPix * 4 )
	{
		ret = IRMODE_4B3 ;
	}
	
	else if ( nWPix * 9 == nPix * 16 )
	{
		ret = IRMODE_16B9 ;
	}
	
	AdjustImageRect( ret, rcRender, rcOut ) ;
	
	return ret ;
}

//////////////////////////////////////////////////////////////////////////
