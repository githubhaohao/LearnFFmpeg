//
// Created by 公众号：字节流动 on 2019/7/10.
//

#ifndef NDK_OPENGLES_3_0_IMAGEDEF_H
#define NDK_OPENGLES_3_0_IMAGEDEF_H

#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include "stdio.h"
#include "sys/stat.h"
#include "stdint.h"
#include "LogUtil.h"

#define IMAGE_FORMAT_RGBA           0x01
#define IMAGE_FORMAT_NV21           0x02
#define IMAGE_FORMAT_NV12           0x03
#define IMAGE_FORMAT_I420           0x04

#define IMAGE_FORMAT_RGBA_EXT       "RGB32"
#define IMAGE_FORMAT_NV21_EXT       "NV21"
#define IMAGE_FORMAT_NV12_EXT       "NV12"
#define IMAGE_FORMAT_I420_EXT       "I420"

typedef struct _tag_NativeRectF
{
	float left;
	float top;
	float right;
	float bottom;
	_tag_NativeRectF()
	{
		left = top = right = bottom = 0.0f;
	}
} RectF;

typedef struct _tag_NativeImage
{
	int width;
	int height;
	int format;
	uint8_t *ppPlane[3];
	int pLineSize[3];
	_tag_NativeImage()
	{
		width = 0;
		height = 0;
		format = 0;
		ppPlane[0] = nullptr;
		ppPlane[1] = nullptr;
		ppPlane[2] = nullptr;
		pLineSize[0] = 0;
		pLineSize[1] = 0;
		pLineSize[2] = 0;
	}
} NativeImage;

class NativeImageUtil
{
public:
	static void AllocNativeImage(NativeImage *pImage)
	{
		if (pImage->height == 0 || pImage->width == 0) return;

		switch (pImage->format)
		{
			case IMAGE_FORMAT_RGBA:
			{
				pImage->ppPlane[0] = static_cast<uint8_t *>(malloc(pImage->width * pImage->height * 4));
				pImage->pLineSize[0] = pImage->width * 4;
				pImage->pLineSize[1] = 0;
				pImage->pLineSize[2] = 0;
			}
				break;
			case IMAGE_FORMAT_NV12:
			case IMAGE_FORMAT_NV21:
			{
				pImage->ppPlane[0] = static_cast<uint8_t *>(malloc(pImage->width * pImage->height * 1.5));
				pImage->ppPlane[1] = pImage->ppPlane[0] + pImage->width * pImage->height;
				pImage->pLineSize[0] = pImage->width;
				pImage->pLineSize[1] = pImage->width;
				pImage->pLineSize[2] = 0;
			}
				break;
			case IMAGE_FORMAT_I420:
			{
				pImage->ppPlane[0] = static_cast<uint8_t *>(malloc(pImage->width * pImage->height * 1.5));
				pImage->ppPlane[1] = pImage->ppPlane[0] + pImage->width * pImage->height;
				pImage->ppPlane[2] = pImage->ppPlane[1] + (pImage->width >> 1) * (pImage->height >> 1);
				pImage->pLineSize[0] = pImage->width;
				pImage->pLineSize[1] = pImage->width / 2;
				pImage->pLineSize[2] = pImage->width / 2;
			}
				break;
			default:
				LOGCATE("NativeImageUtil::AllocNativeImage do not support the format. Format = %d", pImage->format);
				break;
		}
	}

	static void FreeNativeImage(NativeImage *pImage)
	{
		if (pImage == nullptr || pImage->ppPlane[0] == nullptr) return;

		free(pImage->ppPlane[0]);
		pImage->ppPlane[0] = nullptr;
		pImage->ppPlane[1] = nullptr;
		pImage->ppPlane[2] = nullptr;
	}

	static void CopyNativeImage(NativeImage *pSrcImg, NativeImage *pDstImg)
	{
	    LOGCATE("NativeImageUtil::CopyNativeImage src[w,h,format]=[%d, %d, %d], dst[w,h,format]=[%d, %d, %d]", pSrcImg->width, pSrcImg->height, pSrcImg->format, pDstImg->width, pDstImg->height, pDstImg->format);
        LOGCATE("NativeImageUtil::CopyNativeImage src[line0,line1,line2]=[%d, %d, %d], dst[line0,line1,line2]=[%d, %d, %d]", pSrcImg->pLineSize[0], pSrcImg->pLineSize[1], pSrcImg->pLineSize[2], pDstImg->pLineSize[0], pDstImg->pLineSize[1], pDstImg->pLineSize[2]);

        if(pSrcImg == nullptr || pSrcImg->ppPlane[0] == nullptr) return;

		if(pSrcImg->format != pDstImg->format ||
		   pSrcImg->width != pDstImg->width ||
		   pSrcImg->height != pDstImg->height)
		{
			LOGCATE("NativeImageUtil::CopyNativeImage invalid params.");
			return;
		}

		if(pDstImg->ppPlane[0] == nullptr) AllocNativeImage(pDstImg);

		switch (pSrcImg->format)
		{
			case IMAGE_FORMAT_I420:
			{
				// y plane
				if(pSrcImg->pLineSize[0] != pDstImg->pLineSize[0]) {
					for (int i = 0; i < pSrcImg->height; ++i) {
						memcpy(pDstImg->ppPlane[0] + i * pDstImg->pLineSize[0], pSrcImg->ppPlane[0] + i * pSrcImg->pLineSize[0], pDstImg->width);
					}
				}
				else
				{
					memcpy(pDstImg->ppPlane[0], pSrcImg->ppPlane[0], pDstImg->pLineSize[0] * pSrcImg->height);
				}

				// u plane
				if(pSrcImg->pLineSize[1] != pDstImg->pLineSize[1]) {
					for (int i = 0; i < pSrcImg->height / 2; ++i) {
						memcpy(pDstImg->ppPlane[1] + i * pDstImg->pLineSize[1], pSrcImg->ppPlane[1] + i * pSrcImg->pLineSize[1], pDstImg->width / 2);
					}
				}
				else
				{
					memcpy(pDstImg->ppPlane[1], pSrcImg->ppPlane[1], pDstImg->pLineSize[1] * pSrcImg->height / 2);
				}

				// v plane
				if(pSrcImg->pLineSize[2] != pDstImg->pLineSize[2]) {
					for (int i = 0; i < pSrcImg->height / 2; ++i) {
						memcpy(pDstImg->ppPlane[2] + i * pDstImg->pLineSize[2], pSrcImg->ppPlane[2] + i * pSrcImg->pLineSize[2], pDstImg->width / 2);
					}
				}
				else
				{
					memcpy(pDstImg->ppPlane[2], pSrcImg->ppPlane[2], pDstImg->pLineSize[2] * pSrcImg->height / 2);
				}
			}
			    break;
			case IMAGE_FORMAT_NV21:
			case IMAGE_FORMAT_NV12:
			{
				// y plane
				if(pSrcImg->pLineSize[0] != pDstImg->pLineSize[0]) {
					for (int i = 0; i < pSrcImg->height; ++i) {
						memcpy(pDstImg->ppPlane[0] + i * pDstImg->pLineSize[0], pSrcImg->ppPlane[0] + i * pSrcImg->pLineSize[0], pDstImg->width);
					}
				}
				else
				{
					memcpy(pDstImg->ppPlane[0], pSrcImg->ppPlane[0], pDstImg->pLineSize[0] * pSrcImg->height);
				}

				// uv plane
				if(pSrcImg->pLineSize[1] != pDstImg->pLineSize[1]) {
					for (int i = 0; i < pSrcImg->height / 2; ++i) {
						memcpy(pDstImg->ppPlane[1] + i * pDstImg->pLineSize[1], pSrcImg->ppPlane[1] + i * pSrcImg->pLineSize[1], pDstImg->width);
					}
				}
				else
				{
					memcpy(pDstImg->ppPlane[1], pSrcImg->ppPlane[1], pDstImg->pLineSize[1] * pSrcImg->height / 2);
				}
			}
				break;
			case IMAGE_FORMAT_RGBA:
			{
				if(pSrcImg->pLineSize[0] != pDstImg->pLineSize[0])
				{
					for (int i = 0; i < pSrcImg->height; ++i) {
						memcpy(pDstImg->ppPlane[0] + i * pDstImg->pLineSize[0], pSrcImg->ppPlane[0] + i * pSrcImg->pLineSize[0], pDstImg->width * 4);
					}
				} else {
					memcpy(pDstImg->ppPlane[0], pSrcImg->ppPlane[0], pSrcImg->pLineSize[0] * pSrcImg->height);
				}
			}
				break;
			default:
			{
				LOGCATE("NativeImageUtil::CopyNativeImage do not support the format. Format = %d", pSrcImg->format);
			}
				break;
		}

	}

	static void DumpNativeImage(NativeImage *pSrcImg, const char *pPath, const char *pFileName)
	{
		if (pSrcImg == nullptr || pPath == nullptr || pFileName == nullptr) return;

		if(access(pPath, 0) == -1)
		{
			mkdir(pPath, 0666);
		}

		char imgPath[256] = {0};
		const char *pExt = nullptr;
		switch (pSrcImg->format)
		{
			case IMAGE_FORMAT_I420:
				pExt = IMAGE_FORMAT_I420_EXT;
				break;
			case IMAGE_FORMAT_NV12:
				pExt = IMAGE_FORMAT_NV12_EXT;
				break;
			case IMAGE_FORMAT_NV21:
				pExt = IMAGE_FORMAT_NV21_EXT;
				break;
			case IMAGE_FORMAT_RGBA:
				pExt = IMAGE_FORMAT_RGBA_EXT;
				break;
			default:
				pExt = "Default";
				break;
		}

		static int index = 0;
		sprintf(imgPath, "%s/IMG_%dx%d_%s_%d.%s", pPath, pSrcImg->width, pSrcImg->height, pFileName, index, pExt);

		FILE *fp = fopen(imgPath, "wb");

		LOGCATE("DumpNativeImage fp=%p, file=%s", fp, imgPath);

		if(fp)
		{
			switch (pSrcImg->format)
			{
				case IMAGE_FORMAT_I420:
				{
					fwrite(pSrcImg->ppPlane[0],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height), 1, fp);
					fwrite(pSrcImg->ppPlane[1],
						   static_cast<size_t>((pSrcImg->width >> 1) * (pSrcImg->height >> 1)), 1, fp);
					fwrite(pSrcImg->ppPlane[2],
							static_cast<size_t>((pSrcImg->width >> 1) * (pSrcImg->height >> 1)),1,fp);
					break;
				}
				case IMAGE_FORMAT_NV21:
				case IMAGE_FORMAT_NV12:
				{
					fwrite(pSrcImg->ppPlane[0],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height), 1, fp);
					fwrite(pSrcImg->ppPlane[1],
						   static_cast<size_t>(pSrcImg->width * (pSrcImg->height >> 1)), 1, fp);
					break;
				}
				case IMAGE_FORMAT_RGBA:
				{
					fwrite(pSrcImg->ppPlane[0],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height * 4), 1, fp);
					break;
				}
				default:
				{
					LOGCATE("DumpNativeImage default");
					break;
				}
			}

			fclose(fp);
			fp = NULL;
		}


	}
};


#endif //NDK_OPENGLES_3_0_IMAGEDEF_H
