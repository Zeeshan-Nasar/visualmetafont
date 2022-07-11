#ifndef PATHVIEW_H
#define PATHVIEW_H

#include <QPainterPath>
#include <QWidget>

class Glyph;

class PathView : public QWidget
{
  Q_OBJECT

public:
  explicit PathView(Glyph* glyph, QWidget* parent = nullptr);

  //QSize minimumSizeHint() const override;
  //QSize sizeHint() const override;

public slots:    
  void glyphChanged(QString name);

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  Glyph* glyph;
  QPainterPath path;
};

#endif // PATHVIEW_H
