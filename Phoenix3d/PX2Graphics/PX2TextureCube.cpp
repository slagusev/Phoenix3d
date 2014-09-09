/*
*
* �ļ�����	��	PX2TextureCube.cpp
*
*/

#include "PX2TextureCube.hpp"
#include "PX2BitHacks.hpp"
#include "PX2Renderer.hpp"
using namespace PX2;

PX2_IMPLEMENT_RTTI(PX2, Texture, TextureCube);
PX2_IMPLEMENT_STREAM(TextureCube);
PX2_IMPLEMENT_FACTORY(TextureCube);
PX2_IMPLEMENT_DEFAULT_NAMES(Texture, TextureCube);
PX2_IMPLEMENT_DEFAULT_STREAM(Texture, TextureCube);

//----------------------------------------------------------------------------
TextureCube::TextureCube (Format tformat, int dimension, int numLevels,
    Buffer::Usage usage)
    :
    Texture(tformat, TT_CUBE, usage, numLevels)
{
    assertion(dimension > 0, "Dimension must be positive\n");
    mDimension[0][0] = dimension;
    mDimension[1][0] = dimension;

    unsigned int logDim = Log2OfPowerOfTwo((unsigned int)dimension);
    int maxLevels = (int)logDim + 1;

    if (numLevels == 0)
    {
        // ʹ�����levels
        mNumLevels = maxLevels;
    }
    else if (numLevels <= maxLevels)
    {
        // ʹ���ض�levels
        mNumLevels = numLevels;
    }
    else
    {
        assertion(false, "Invalid number of levels\n");
    }

    ComputeNumLevelBytes();
    mData = new1<char>(mNumTotalBytes);
}
//----------------------------------------------------------------------------
TextureCube::~TextureCube ()
{
    Renderer::UnbindAll(this);
}
//----------------------------------------------------------------------------
void TextureCube::GenerateMipmaps ()
{
	// �������������������������������������������Ⱦ�����ͷţ�֮�����¼��㡣
    unsigned int logDim = Log2OfPowerOfTwo((unsigned int)mDimension[0][0]);
    int maxLevels = (int)logDim + 1;
    int face, faceOffset, level;
    bool retainBindings = true;
    if (mNumLevels != maxLevels)
    {
        retainBindings = false;
        Renderer::UnbindAll(this);
        mNumLevels = maxLevels;
        int oldNumTotalBytes = mNumTotalBytes/6;
        ComputeNumLevelBytes();

        char* newData = new1<char>(mNumTotalBytes);
        for (face = 0; face < 6; ++face)
        {
            int oldFaceOffset = face*oldNumTotalBytes;
            faceOffset = face*mNumTotalBytes/6;
            memcpy(newData + faceOffset, mData + oldFaceOffset,
                mNumLevelBytes[0]);
        }
        delete1(mData);
        mData = newData;
    }

	// ��ʱbuffer��������mipmap
    Float4* rgba = new1<Float4>(mDimension[0][0]*mDimension[1][0]);

    for (face = 0; face < 6; ++face)
    {
        faceOffset = face*mNumTotalBytes/6;
        char* texels = mData + faceOffset;
        int dimension = mDimension[0][0];
        for (level = 1; level < mNumLevels; ++level)
        {
            char* texelsNext = mData + faceOffset + mLevelOffsets[level];
            int dimensionNext = mDimension[0][level];

            GenerateNextMipmap(dimension, texels, dimensionNext, texelsNext,
                rgba);

            dimension = dimensionNext;
            texels = texelsNext;
        }
    }

    delete1(rgba);

    if (retainBindings)
    {
        for (face = 0; face < 6; ++face)
        {
            for (level = 0; level < mNumLevels; ++level)
            {
                Renderer::UpdateAll(this, face, level);
            }
        }
    }
}
//----------------------------------------------------------------------------
bool TextureCube::HasMipmaps () const
{
    unsigned int logDim = Log2OfPowerOfTwo((unsigned int)mDimension[0][0]);
    int maxLevels = (int)logDim + 1;
    return mNumLevels == maxLevels;
}
//----------------------------------------------------------------------------
char* TextureCube::GetData (int face, int level) const
{
    if (mData && 0 <= face && face < 6 && 0 <= level && level < mNumLevels)
    {
        int faceOffset = face*mNumTotalBytes/6;
        return mData + faceOffset + mLevelOffsets[level];
    }

    assertion(false, "Null pointer or invalid level in GetData\n");
    return 0;
}
//----------------------------------------------------------------------------
void TextureCube::ComputeNumLevelBytes ()
{
    if (mFormat == TF_R32F
    ||  mFormat == TF_G32R32F
    ||  mFormat == TF_A32B32G32R32F)
    {
        if (mNumLevels > 1)
        {
            assertion(false, "No mipmaps for 32-bit float textures\n");
            mNumLevels = 1;
        }
    }
    else if (mFormat == TF_D24S8)
    {
        if (mNumLevels > 1)
        {
            assertion(false, "No mipmaps for 2D depth textures\n");
            mNumLevels = 1;
        }
    }

    int dim0 = mDimension[0][0];
    int dim1 = mDimension[1][0];
    int level;
    mNumTotalBytes = 0;

    if (mFormat == TF_DXT1)
    {
        for (level = 0; level < mNumLevels; ++level)
        {
            int max0 = dim0/4;
            if (max0 < 1)
            {
                max0 = 1;
            }
            int max1 = dim1/4;
            if (max1 < 1)
            {
                max1 = 1;
            }

            mNumLevelBytes[level] = 8*max0*max1;
            mNumTotalBytes += mNumLevelBytes[level];
            mDimension[0][level] = dim0;
            mDimension[1][level] = dim1;

            if (dim0 > 1)
            {
                dim0 >>= 1;
            }
            if (dim1 > 1)
            {
                dim1 >>= 1;
            }
        }
    }
    else if (mFormat == TF_DXT3 || mFormat == TF_DXT5)
    {
        for (level = 0; level < mNumLevels; ++level)
        {
            int max0 = dim0/4;
            if (max0 < 1)
            {
                max0 = 1;
            }
            int max1 = dim1/4;
            if (max1 < 1)
            {
                max1 = 1;
            }

            mNumLevelBytes[level] = 16*max0*max1;
            mNumTotalBytes += mNumLevelBytes[level];
            mDimension[0][level] = dim0;
            mDimension[1][level] = dim1;

            if (dim0 > 1)
            {
                dim0 >>= 1;
            }
            if (dim1 > 1)
            {
                dim1 >>= 1;
            }
        }
    }
    else
    {
        for (level = 0; level < mNumLevels; ++level)
        {
            mNumLevelBytes[level] = msPixelSize[mFormat]*dim0*dim1;
            mNumTotalBytes += mNumLevelBytes[level];
            mDimension[0][level] = dim0;
            mDimension[1][level] = dim1;

            if (dim0 > 1)
            {
                dim0 >>= 1;
            }
            if (dim1 > 1)
            {
                dim1 >>= 1;
            }
        }
    }

    mNumTotalBytes *= 6;

    mLevelOffsets[0] = 0;
    for (level = 0; level < mNumLevels-1; ++level)
    {
        mLevelOffsets[level+1] = mLevelOffsets[level] + mNumLevelBytes[level];
    }
}
//----------------------------------------------------------------------------
void TextureCube::GenerateNextMipmap (int dimension, const char* texels,
    int dimensionNext, char* texelsNext, Float4* rgba)
{
    // ��ԭʼ��ʽת����32-bitRGBA
    int numTexels = dimension*dimension;
    msConvertFrom[mFormat](numTexels, texels, rgba);

    // ����miplevel
    for (int i1 = 0; i1 < dimensionNext; ++i1)
    {
        for (int i0 = 0; i0 < dimensionNext; ++i0)
        {
            int j = i0 + dimensionNext*i1;
            int base = 2*(i0 + dimension*i1);
            for (int c = 0; c < 4; ++c)
            {
                rgba[j][c] = 0.25f*(
                    rgba[base][c] +
                    rgba[base + 1][c] +
                    rgba[base + dimension][c] +
                    rgba[base + dimension + 1][c]);
            }
        }
    }

    // ��32-bit��ʽת����ԭʼ��ʽ
    int numTexelsNext = dimensionNext*dimensionNext;
    msConvertTo[mFormat](numTexelsNext, rgba, texelsNext);
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ���ݶ�ȡ
//----------------------------------------------------------------------------
TextureCube* TextureCube::LoadPXtf (const std::string& name, int mode)
{
    Texture* texture = Texture::LoadPXtf(name, mode);
    if (texture->IsExactly(TextureCube::TYPE))
    {
        return (TextureCube*)texture;
    }
    return 0;
}
//----------------------------------------------------------------------------