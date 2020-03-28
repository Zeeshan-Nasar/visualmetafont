﻿/*
 * Copyright (c) 2015-2020 Amine Anane. http: //digitalkhatt/license
 * This file is part of DigitalKhatt.
 *
 * DigitalKhatt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * DigitalKhatt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with DigitalKhatt. If not, see
 * <https: //www.gnu.org/licenses />.
*/

#pragma once
#ifndef GLYPHSCENE_H
#define GLYPHSCENE_H
#include <QGraphicsScene>
#include <QMap>
#include <QLabel>
#include "glyph.hpp"
#include "ruleritem.hpp"
#include "contouritem.hpp"
#include "imageitem.hpp"
#include "guidesitem.hpp"
#include "componentitem.hpp"


class Glyph;

class GlyphScene : public QGraphicsScene {
	Q_OBJECT
		friend class GlyphView;
public:

	enum class ItemFlags {
		None = 0,
		ImageEnabled = 1,
		ImageVisible = 2,
		Fill = 4
	};

	GlyphScene(QObject * parent = Q_NULLPTR);
	~GlyphScene();
	enum Mode { MoveItem, Ruler, AddPoint };
	QLabel * pointerPosition;
	void setGlyph(Glyph * glyph);

	void setImageVisible(bool  visible);
	void setImageEnable(bool  enable);
	void setFillEnable(bool  enable);

public slots:
	void setMode(Mode mode);


protected:
	void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) Q_DECL_OVERRIDE;
	void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) Q_DECL_OVERRIDE;

private slots:
	void recordGlyphChange(QString name);
	void glyphValueChanged(QString name, bool structureChanged);
	void selectionChangedSlot();
private:

	void addPointAfterPoint(KnotControlledItem* point, QPointF newpoint, QString dir);
	void addnewPath(QPointF newpoint, QString dir);

	QMap<QString, QVariant> m_oldValues;
	QMap<QString, QVariant> m_newValues;
	QMap<int, QMap<int, Glyph::Knot> >  old_controlledPaths;
	QMap<int, QMap<int, Glyph::Knot> >  new_controlledPaths;
	Glyph* m_glyph;
	Mode myMode;
	RulerItem* ruler;
	bool ismoving;
	bool wasMouseMoveEvent;

	void loadImage(Glyph::ImageInfo imageInfo);
	ContourItem * contour;

	GuidesItem * guides;
	QVector<ComponentItem*> components;
	ImageItem * image;

	ItemFlags itemFlags;
};

constexpr GlyphScene::ItemFlags operator|(GlyphScene::ItemFlags a, GlyphScene::ItemFlags b)
{
	return static_cast<GlyphScene::ItemFlags>(static_cast<int>(a) | static_cast<int>(b));
}

constexpr GlyphScene::ItemFlags operator&(GlyphScene::ItemFlags a, GlyphScene::ItemFlags b)
{
	return static_cast<GlyphScene::ItemFlags>(static_cast<int>(a)&static_cast<int>(b));
}

constexpr GlyphScene::ItemFlags operator~(GlyphScene::ItemFlags a)
{
	return static_cast<GlyphScene::ItemFlags>(~static_cast<int>(a));
}

#endif // GLYPHSCENE_H