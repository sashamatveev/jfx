/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#include "Length.h"
#include "LengthSize.h"
#include "RectEdges.h"
#include "WindRule.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/TypeCasts.h>
#include <wtf/Vector.h>

namespace WTF {
class TextStream;
}

namespace WebCore {

struct BlendingContext;
class FloatRect;
class Path;
class RenderBox;
class SVGPathByteStream;

enum class CoordinateAffinity : uint8_t {
    Relative, Absolute
};

class BasicShape : public RefCounted<BasicShape> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~BasicShape() = default;

    enum class Type : uint8_t {
        Polygon,
        Path,
        Circle,
        Ellipse,
        Inset,
        Rect,
        Xywh,
        Shape
    };

    virtual Ref<BasicShape> clone() const = 0;

    virtual Type type() const = 0;

    virtual Path path(const FloatRect&) const = 0;
    virtual WindRule windRule() const { return WindRule::NonZero; }

    virtual bool canBlend(const BasicShape&) const = 0;
    virtual Ref<BasicShape> blend(const BasicShape& from, const BlendingContext&) const = 0;

    virtual bool operator==(const BasicShape&) const = 0;

    virtual void dump(TextStream&) const = 0;
};

class BasicShapeCenterCoordinate {
public:
    enum class Direction : bool {
        TopLeft,
        BottomRight
    };

    BasicShapeCenterCoordinate()
    {
        updateComputedLength();
    }

    BasicShapeCenterCoordinate(Direction direction, Length&& length)
        : m_direction(direction)
        , m_length(WTFMove(length))
    {
        updateComputedLength();
    }

    Direction direction() const { return m_direction; }
    const Length& length() const { return m_length; }
    const Length& computedLength() const { return m_computedLength; }

    BasicShapeCenterCoordinate blend(const BasicShapeCenterCoordinate& from, const BlendingContext& context) const
    {
        return BasicShapeCenterCoordinate(Direction::TopLeft, WebCore::blend(from.m_computedLength, m_computedLength, context));
    }

    bool operator==(const BasicShapeCenterCoordinate& other) const
    {
        return m_direction == other.m_direction
            && m_length == other.m_length
            && m_computedLength == other.m_computedLength;
    }

private:
    WEBCORE_EXPORT void updateComputedLength();

    Direction m_direction { Direction::TopLeft };
    Length m_length { LengthType::Undefined };
    Length m_computedLength;
};

class BasicShapeRadius {
public:
    enum class Type : uint8_t {
        Value,
        ClosestSide,
        FarthestSide,
        ClosestCorner,
        FarthestCorner
    };

    BasicShapeRadius() = default;

    explicit BasicShapeRadius(Length v)
        : m_value(v)
        , m_type(Type::Value)
    { }
    explicit BasicShapeRadius(Type t)
        : m_value(LengthType::Undefined)
        , m_type(t)
    { }
    explicit BasicShapeRadius(Length&& v, Type t)
        : m_value(WTFMove(v))
        , m_type(t)
    { }

    const Length& value() const { return m_value; }
    Type type() const { return m_type; }

    bool canBlend(const BasicShapeRadius& other) const
    {
        // FIXME determine how to interpolate between keywords. See bug 125108.
        return m_type == Type::Value && other.type() == Type::Value;
    }

    BasicShapeRadius blend(const BasicShapeRadius& from, const BlendingContext& context) const
    {
        if (m_type != Type::Value || from.type() != Type::Value)
            return BasicShapeRadius(from);

        return BasicShapeRadius(WebCore::blend(from.value(), value(), context));
    }

    bool operator==(const BasicShapeRadius& other) const
    {
        return m_value == other.m_value && m_type == other.m_type;
    }

private:
    Length m_value { LengthType::Undefined };
    Type m_type { Type::ClosestSide };
};

class BasicShapeCircleOrEllipse : public BasicShape {
public:
    void setPositionWasOmitted(bool flag) { m_centerWasOmitted = flag; }
    bool positionWasOmitted() const { return m_centerWasOmitted; }
    virtual Path pathForCenterCoordinate(const FloatRect&, FloatPoint) const = 0;

private:
    bool m_centerWasOmitted = false;
};

class BasicShapeCircle final : public BasicShapeCircleOrEllipse {
public:
    static Ref<BasicShapeCircle> create() { return adoptRef(*new BasicShapeCircle); }
    WEBCORE_EXPORT static Ref<BasicShapeCircle> create(BasicShapeCenterCoordinate&& centerX, BasicShapeCenterCoordinate&& centerY, BasicShapeRadius&&);

    Ref<BasicShape> clone() const final;

    const BasicShapeCenterCoordinate& centerX() const { return m_centerX; }
    const BasicShapeCenterCoordinate& centerY() const { return m_centerY; }

    const BasicShapeRadius& radius() const { return m_radius; }

    float floatValueForRadiusInBox(FloatSize boxSize, FloatPoint center) const;

    void setCenterX(BasicShapeCenterCoordinate centerX) { m_centerX = WTFMove(centerX); }
    void setCenterY(BasicShapeCenterCoordinate centerY) { m_centerY = WTFMove(centerY); }

    void setRadius(BasicShapeRadius radius) { m_radius = WTFMove(radius); }

private:
    BasicShapeCircle() = default;
    BasicShapeCircle(BasicShapeCenterCoordinate&& centerX, BasicShapeCenterCoordinate&& centerY, BasicShapeRadius&&);

    Type type() const final { return Type::Circle; }

    Path path(const FloatRect&) const final;
    Path pathForCenterCoordinate(const FloatRect&, FloatPoint) const final;

    bool canBlend(const BasicShape&) const final;
    Ref<BasicShape> blend(const BasicShape& from, const BlendingContext&) const final;

    bool operator==(const BasicShape&) const final;

    void dump(TextStream&) const final;

    BasicShapeCenterCoordinate m_centerX;
    BasicShapeCenterCoordinate m_centerY;
    BasicShapeRadius m_radius;
};

class BasicShapeEllipse final : public BasicShapeCircleOrEllipse {
public:
    static Ref<BasicShapeEllipse> create() { return adoptRef(*new BasicShapeEllipse); }
    WEBCORE_EXPORT static Ref<BasicShapeEllipse> create(BasicShapeCenterCoordinate&& centerX, BasicShapeCenterCoordinate&& centerY, BasicShapeRadius&& radiusX, BasicShapeRadius&& radiusY);

    Ref<BasicShape> clone() const final;

    const BasicShapeCenterCoordinate& centerX() const { return m_centerX; }
    const BasicShapeCenterCoordinate& centerY() const { return m_centerY; }
    const BasicShapeRadius& radiusX() const { return m_radiusX; }
    const BasicShapeRadius& radiusY() const { return m_radiusY; }

    FloatSize floatSizeForRadiusInBox(FloatSize boxSize, FloatPoint center) const;

    void setCenterX(BasicShapeCenterCoordinate centerX) { m_centerX = WTFMove(centerX); }
    void setCenterY(BasicShapeCenterCoordinate centerY) { m_centerY = WTFMove(centerY); }
    void setRadiusX(BasicShapeRadius radiusX) { m_radiusX = WTFMove(radiusX); }
    void setRadiusY(BasicShapeRadius radiusY) { m_radiusY = WTFMove(radiusY); }

private:
    BasicShapeEllipse() = default;
    BasicShapeEllipse(BasicShapeCenterCoordinate&& centerX, BasicShapeCenterCoordinate&& centerY, BasicShapeRadius&& radiusX, BasicShapeRadius&& radiusY);

    Type type() const final { return Type::Ellipse; }

    Path path(const FloatRect&) const final;
    Path pathForCenterCoordinate(const FloatRect&, FloatPoint) const final;

    bool canBlend(const BasicShape&) const final;
    Ref<BasicShape> blend(const BasicShape& from, const BlendingContext&) const final;

    bool operator==(const BasicShape&) const final;

    void dump(TextStream&) const final;

    BasicShapeCenterCoordinate m_centerX;
    BasicShapeCenterCoordinate m_centerY;
    BasicShapeRadius m_radiusX;
    BasicShapeRadius m_radiusY;
};

class BasicShapePolygon final : public BasicShape {
public:
    static Ref<BasicShapePolygon> create() { return adoptRef(*new BasicShapePolygon); }
    WEBCORE_EXPORT static Ref<BasicShapePolygon> create(WindRule, Vector<Length>&& values);

    Ref<BasicShape> clone() const final;

    const Vector<Length>& values() const { return m_values; }
    const Length& getXAt(unsigned i) const { return m_values[2 * i]; }
    const Length& getYAt(unsigned i) const { return m_values[2 * i + 1]; }

    void setWindRule(WindRule windRule) { m_windRule = windRule; }
    void appendPoint(Length x, Length y) { m_values.append(WTFMove(x)); m_values.append(WTFMove(y)); }

    WindRule windRule() const final { return m_windRule; }

private:
    BasicShapePolygon() = default;
    BasicShapePolygon(WindRule, Vector<Length>&& values);

    Type type() const final { return Type::Polygon; }

    Path path(const FloatRect&) const final;

    bool canBlend(const BasicShape&) const final;
    Ref<BasicShape> blend(const BasicShape& from, const BlendingContext&) const final;

    bool operator==(const BasicShape&) const final;

    void dump(TextStream&) const final;

    WindRule m_windRule { WindRule::NonZero };
    Vector<Length> m_values;
};

class BasicShapePath final : public BasicShape {
public:
    static Ref<BasicShapePath> create(std::unique_ptr<SVGPathByteStream>&& byteStream)
    {
        return adoptRef(*new BasicShapePath(WTFMove(byteStream)));
    }

    WEBCORE_EXPORT static Ref<BasicShapePath> create(std::unique_ptr<SVGPathByteStream>&&, float zoom, WindRule);

    virtual ~BasicShapePath();

    Ref<BasicShape> clone() const final;

    void setWindRule(WindRule windRule) { m_windRule = windRule; }
    WindRule windRule() const final { return m_windRule; }

    void setZoom(float z) { m_zoom = z; }
    float zoom() const { return m_zoom; }

    const SVGPathByteStream* pathData() const { return m_byteStream.get(); }
    const std::unique_ptr<SVGPathByteStream>& byteStream() const { return m_byteStream; }

    Path path(const FloatRect&) const final;

    bool canBlend(const BasicShape&) const final;
    Ref<BasicShape> blend(const BasicShape& from, const BlendingContext&) const final;

private:
    BasicShapePath(std::unique_ptr<SVGPathByteStream>&&);
    BasicShapePath(std::unique_ptr<SVGPathByteStream>&&, float zoom, WindRule);

    Type type() const final { return Type::Path; }

    bool operator==(const BasicShape&) const final;

    void dump(TextStream&) const final;

    std::unique_ptr<SVGPathByteStream> m_byteStream;
    float m_zoom { 1 };
    WindRule m_windRule { WindRule::NonZero };
};

class BasicShapeInset final : public BasicShape {
public:
    static Ref<BasicShapeInset> create() { return adoptRef(*new BasicShapeInset); }
    WEBCORE_EXPORT static Ref<BasicShapeInset> create(Length&& right, Length&& top, Length&& bottom, Length&& left, LengthSize&& topLeftRadius, LengthSize&& topRightRadius, LengthSize&& bottomRightRadius, LengthSize&& bottomLeftRadius);

    Ref<BasicShape> clone() const final;

    const Length& top() const { return m_top; }
    const Length& right() const { return m_right; }
    const Length& bottom() const { return m_bottom; }
    const Length& left() const { return m_left; }

    const LengthSize& topLeftRadius() const { return m_topLeftRadius; }
    const LengthSize& topRightRadius() const { return m_topRightRadius; }
    const LengthSize& bottomRightRadius() const { return m_bottomRightRadius; }
    const LengthSize& bottomLeftRadius() const { return m_bottomLeftRadius; }

    void setTop(Length&& top) { m_top = WTFMove(top); }
    void setRight(Length&& right) { m_right = WTFMove(right); }
    void setBottom(Length&& bottom) { m_bottom = WTFMove(bottom); }
    void setLeft(Length&& left) { m_left = WTFMove(left); }

    void setTopLeftRadius(LengthSize&& radius) { m_topLeftRadius = WTFMove(radius); }
    void setTopRightRadius(LengthSize&& radius) { m_topRightRadius = WTFMove(radius); }
    void setBottomRightRadius(LengthSize&& radius) { m_bottomRightRadius = WTFMove(radius); }
    void setBottomLeftRadius(LengthSize&& radius) { m_bottomLeftRadius = WTFMove(radius); }

private:
    BasicShapeInset() = default;
    BasicShapeInset(Length&& right, Length&& top, Length&& bottom, Length&& left, LengthSize&& topLeftRadius, LengthSize&& topRightRadius, LengthSize&& bottomRightRadius, LengthSize&& bottomLeftRadius);

    Type type() const override { return Type::Inset; }

    Path path(const FloatRect&) const override;

    bool canBlend(const BasicShape&) const override;
    Ref<BasicShape> blend(const BasicShape& from, const BlendingContext&) const override;

    bool operator==(const BasicShape&) const override;

    void dump(TextStream&) const final;

    Length m_right;
    Length m_top;
    Length m_bottom;
    Length m_left;

    LengthSize m_topLeftRadius;
    LengthSize m_topRightRadius;
    LengthSize m_bottomRightRadius;
    LengthSize m_bottomLeftRadius;
};

class BasicShapeRect final : public BasicShape {
public:
    static Ref<BasicShapeRect> create() { return adoptRef(*new BasicShapeRect); }
    WEBCORE_EXPORT static Ref<BasicShapeRect> create(Length&& top, Length&& right, Length&& bottom, Length&& left, LengthSize&& topLeftRadius, LengthSize&& topRightRadius, LengthSize&& bottomRightRadius, LengthSize&& bottomLeftRadius);

    Ref<BasicShape> clone() const final;

    const Length& top() const { return m_edges.top(); }
    const Length& right() const { return m_edges.right(); }
    const Length& bottom() const { return m_edges.bottom(); }
    const Length& left() const { return m_edges.left(); }

    const LengthSize& topLeftRadius() const { return m_topLeftRadius; }
    const LengthSize& topRightRadius() const { return m_topRightRadius; }
    const LengthSize& bottomRightRadius() const { return m_bottomRightRadius; }
    const LengthSize& bottomLeftRadius() const { return m_bottomLeftRadius; }

    void setTop(Length&& top) { m_edges.setTop(WTFMove(top)); }
    void setRight(Length&& right) { m_edges.setRight(WTFMove(right)); }
    void setBottom(Length&& bottom) { m_edges.setBottom(WTFMove(bottom)); }
    void setLeft(Length&& left) { m_edges.setLeft(WTFMove(left)); }

    void setTopLeftRadius(LengthSize&& radius) { m_topLeftRadius = WTFMove(radius); }
    void setTopRightRadius(LengthSize&& radius) { m_topRightRadius = WTFMove(radius); }
    void setBottomRightRadius(LengthSize&& radius) { m_bottomRightRadius = WTFMove(radius); }
    void setBottomLeftRadius(LengthSize&& radius) { m_bottomLeftRadius = WTFMove(radius); }

private:
    BasicShapeRect() = default;
    BasicShapeRect(RectEdges<Length>&&, LengthSize&& topLeftRadius, LengthSize&& topRightRadius, LengthSize&& bottomRightRadius, LengthSize&& bottomLeftRadius);
    BasicShapeRect(Length&& top, Length&& right, Length&& bottom, Length&& left, LengthSize&& topLeftRadius, LengthSize&& topRightRadius, LengthSize&& bottomRightRadius, LengthSize&& bottomLeftRadius);

    Type type() const final { return Type::Rect; }

    Path path(const FloatRect&) const final;

    bool canBlend(const BasicShape&) const final;
    Ref<BasicShape> blend(const BasicShape& from, const BlendingContext&) const final;

    bool operator==(const BasicShape&) const final;

    void dump(TextStream&) const final;

    RectEdges<Length> m_edges;

    LengthSize m_topLeftRadius;
    LengthSize m_topRightRadius;
    LengthSize m_bottomRightRadius;
    LengthSize m_bottomLeftRadius;
};

class BasicShapeXywh final : public BasicShape {
public:
    static Ref<BasicShapeXywh> create() { return adoptRef(*new BasicShapeXywh); }
    WEBCORE_EXPORT static Ref<BasicShapeXywh> create(Length&& insetX, Length&& insetY, Length&& width, Length&& height, LengthSize&& topLeftRadius, LengthSize&& topRightRadius, LengthSize&& bottomRightRadius, LengthSize&& bottomLeftRadius);

    Ref<BasicShape> clone() const final;

    const Length& insetX() const { return m_insetX; }
    const Length& insetY() const { return m_insetY; }
    const Length& width() const { return m_width; }
    const Length& height() const { return m_height; }

    const LengthSize& topLeftRadius() const { return m_topLeftRadius; }
    const LengthSize& topRightRadius() const { return m_topRightRadius; }
    const LengthSize& bottomRightRadius() const { return m_bottomRightRadius; }
    const LengthSize& bottomLeftRadius() const { return m_bottomLeftRadius; }

    void setInsetX(Length&& insetX) { m_insetX = WTFMove(insetX); }
    void setInsetY(Length&& insetY) { m_insetY = WTFMove(insetY); }
    void setWidth(Length&& width) { m_width = WTFMove(width); }
    void setHeight(Length&& height) { m_height = WTFMove(height); }

    void setTopLeftRadius(LengthSize&& radius) { m_topLeftRadius = WTFMove(radius); }
    void setTopRightRadius(LengthSize&& radius) { m_topRightRadius = WTFMove(radius); }
    void setBottomRightRadius(LengthSize&& radius) { m_bottomRightRadius = WTFMove(radius); }
    void setBottomLeftRadius(LengthSize&& radius) { m_bottomLeftRadius = WTFMove(radius); }

private:
    BasicShapeXywh() = default;
    BasicShapeXywh(Length&& insetX, Length&& insetY, Length&& width, Length&& height, LengthSize&& topLeftRadius, LengthSize&& topRightRadius, LengthSize&& bottomRightRadius, LengthSize&& bottomLeftRadius);

    Type type() const final { return Type::Xywh; }

    Path path(const FloatRect&) const final;

    bool canBlend(const BasicShape&) const final;
    Ref<BasicShape> blend(const BasicShape& from, const BlendingContext&) const final;

    bool operator==(const BasicShape&) const final;

    void dump(TextStream&) const final;

    Length m_insetX;
    Length m_insetY;
    Length m_width;
    Length m_height;

    LengthSize m_topLeftRadius;
    LengthSize m_topRightRadius;
    LengthSize m_bottomRightRadius;
    LengthSize m_bottomLeftRadius;
};

WTF::TextStream& operator<<(WTF::TextStream&, CoordinateAffinity);
WTF::TextStream& operator<<(WTF::TextStream&, const BasicShapeRadius&);
WTF::TextStream& operator<<(WTF::TextStream&, const BasicShapeCenterCoordinate&);
WTF::TextStream& operator<<(WTF::TextStream&, const BasicShape&);

} // namespace WebCore

#define SPECIALIZE_TYPE_TRAITS_BASIC_SHAPE_CIRCULAR(ToValueTypeName, predicate1, predicate2) \
SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::ToValueTypeName) \
    static bool isType(const WebCore::BasicShape& basicShape) { return basicShape.type() == WebCore::predicate1 || basicShape.type() == WebCore::predicate2; } \
SPECIALIZE_TYPE_TRAITS_END()

#define SPECIALIZE_TYPE_TRAITS_BASIC_SHAPE(ToValueTypeName, predicate) \
SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::ToValueTypeName) \
    static bool isType(const WebCore::BasicShape& basicShape) { return basicShape.type() == WebCore::predicate; } \
SPECIALIZE_TYPE_TRAITS_END()

SPECIALIZE_TYPE_TRAITS_BASIC_SHAPE(BasicShapeCircle, BasicShape::Type::Circle)
SPECIALIZE_TYPE_TRAITS_BASIC_SHAPE(BasicShapeEllipse, BasicShape::Type::Ellipse)
SPECIALIZE_TYPE_TRAITS_BASIC_SHAPE(BasicShapePolygon, BasicShape::Type::Polygon)
SPECIALIZE_TYPE_TRAITS_BASIC_SHAPE(BasicShapePath, BasicShape::Type::Path)
SPECIALIZE_TYPE_TRAITS_BASIC_SHAPE(BasicShapeInset, BasicShape::Type::Inset)
SPECIALIZE_TYPE_TRAITS_BASIC_SHAPE(BasicShapeRect, BasicShape::Type::Rect)
SPECIALIZE_TYPE_TRAITS_BASIC_SHAPE(BasicShapeXywh, BasicShape::Type::Xywh)
SPECIALIZE_TYPE_TRAITS_BASIC_SHAPE_CIRCULAR(BasicShapeCircleOrEllipse, BasicShape::Type::Circle, BasicShape::Type::Ellipse);
