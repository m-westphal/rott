/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "../rt_def.h"
#include "../watcom.h"
#include "../rt_draw.h"
#include "../rt_door.h"
#include <stdlib.h>


typedef struct {
	int x;
	int y;
} point;


static point cast_ray(const int c_vx, const int c_vy) {
	point mypos;
	int xtilestep,ytilestep;
	int snx,sny;
	int incr[2];
	int thedir[2];
	int cnt;
	int grid[2];
	int index;

	/* set snx, sny to player pos inside current grid*/
	snx=viewx&0xffff;
	sny=viewy&0xffff;

	if (c_vx>0) {
		thedir[0]=1;
		xtilestep=0x80;
		snx^=0xffff;
		incr[1]=-c_vx;
	}
	else {
		thedir[0]=-1;
		xtilestep=-0x80;
		incr[1]=c_vx;
	}
	if (c_vy>0) {
		thedir[1]=1;
		ytilestep=1;
		sny^=0xffff;
		incr[0]=c_vy;
	}
	else {
		thedir[1]=-1;
		ytilestep=-1;
		incr[0]=-c_vy;
	}

	cnt=FixedMul(snx,incr[0])+FixedMul(sny,incr[1]);
	grid[0]=viewx>>16;
	grid[1]=viewy>>16;

	/* cast the ray */
	do {
		int tile;

		index=(cnt>=0);
		cnt+=incr[index];
		spotvis[grid[0]][grid[1]]=1;
		grid[index]+=thedir[index];

		if ((tile=tilemap[grid[0]][grid[1]])!=0) {
			if (tile&0x8000) {
				if ( (!(tile&0x4000)) && (doorobjlist[tile&0x3ff]->action==dr_closed)) {
					spotvis[grid[0]][grid[1]]=1;
					if (doorobjlist[tile&0x3ff]->flags&DF_MULTI)
						MakeWideDoorVisible(tile&0x3ff);
					do {
						index=(cnt>=0);
						cnt+=incr[index];
						grid[index]+=thedir[index];
						if ((tilemap[grid[0]][grid[1]]!=0) && (!(tilemap[grid[0]][grid[1]]&0x8000)) )
							break;
					}
					while (1);
					break;
				}
				else
					continue;
			}
			else {
				mapseen[grid[0]][grid[1]]=1;
				break;
			}
		}
	}
	while (1);

	mypos.x = grid[0];
	mypos.y = grid[1];

	return mypos;
}


static inline int distance(const point r1, const point r2) {
	return (r1.x-r2.x || r1.y-r2.y);	/* distance == 0 ? */
}


#if (DEBUG==1)
static unsigned int minicasts;
#endif


static void mini_cast (const int c_vx, const int c_vy, const point min, const point max, const float adv) {
	int	bc_vx, bc_vy;
	point	cur;

	if (adv < 0.001f) return;

	bc_vx = c_vx - (int) ( adv * (float) viewsin);
	bc_vy = c_vy - (int) ( adv * (float) viewcos);

	cur = cast_ray(bc_vx, bc_vy);

#if (DEBUG==1)
	minicasts++;
//	printf("%d, %d, %d, %f\n", minicasts, pdistance(max,cur), pdistance(min,cur), adv);
#endif

	if (distance (cur, max)) mini_cast(c_vx, c_vy, cur, max, adv/2.0f );
	if (distance (min, cur)) mini_cast(bc_vx, bc_vy, min, cur, adv/2.0f );
}


void Refresh ( void ) {
	int	c_vx = c_startx - (80 * viewsin), c_vy = c_starty - (80 * viewcos);
	point	min,max;

	min = cast_ray(c_vx, c_vy);

#if (DEBUG==1)
	minicasts = 0;
#endif

	mapseen[viewx>>16][viewy>>16] = 1;

	c_vx += (320+80+80) * viewsin;
	c_vy += (320+80+80) * viewcos;

	max = cast_ray(c_vx, c_vy);
	mini_cast(c_vx, c_vy, min, max, (320+80+80)/2);

#if (DEBUG==1)
	if(minicasts > 1000) printf("DEBUG: opengl/engine.c: Massive casts: %d\n", minicasts);
#endif
}
