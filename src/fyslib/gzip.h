/*
 * gzip.h
 *
 *  Created on: Sep 5, 2017
 *      Author: root
 */

#ifndef FYSLIB_GZIP_H_
#define FYSLIB_GZIP_H_

#include <zlib.h>

/* Compress gzip data */
/* data 原数据 ndata 原数据长度 zdata 压缩后数据 nzdata 压缩后长度 */
int gzcompress(Bytef *data, uLong ndata,Bytef *zdata, uLong *nzdata);
/* Uncompress gzip data */
/* zdata 数据 nzdata 原数据长度 data 解压后数据 ndata 解压后长度 */
int gzdecompress(Byte *zdata, uLong nzdata,Byte *data, uLong *ndata);



#endif /* FYSLIB_GZIP_H_ */
