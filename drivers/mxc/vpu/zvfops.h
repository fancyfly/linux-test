/*
 * Copyright 2012-2015 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */


#ifndef __ZVFOPS_H__
#define __ZVFOPS_H__

int zvf_open(unsigned long *pdwHandle, unsigned char *pFName, unsigned int dwReadOnly);
int zvf_close(unsigned long dwHandle);

int zvf_write(unsigned long dwHandle, unsigned char *pBuff, unsigned int dwLen);
int zvf_read(unsigned long dwHandle, unsigned char *pBuff, unsigned int dwLen);
int zvf_load(char *pszFileName, 
			  unsigned char **ppucaBuff, 
			  unsigned int *pulBuffLen
			  );

#endif //__ZVFOPS_H__





