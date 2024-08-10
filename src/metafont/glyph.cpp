/*
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

#include "qstringbuilder.h"

#include "glyph.hpp"
#include "font.hpp"
#include "commands.h"

#include "GlyphParser/glyphdriver.h"
#include "qcoreevent.h"
#include  <cmath>

#include "metafont.h"

Glyph::Param::Param(const Glyph::Param& a) {
  name = a.name;
  position = a.position;
  applytosubpath = a.applytosubpath;
  applytopoint = a.applytopoint;
  isEquation = a.isEquation;
  isInControllePath = a.isInControllePath;
  affects = a.affects;
  value = a.value;
  expr = a.expr->clone();
}
Glyph::Param::Param(Glyph::Param&& a) {
  name = a.name;
  position = a.position;
  applytosubpath = a.applytosubpath;
  applytopoint = a.applytopoint;
  isEquation = a.isEquation;
  isInControllePath = a.isInControllePath;
  affects = a.affects;
  value = a.value;
  expr = std::move(a.expr);
}
Glyph::Param& Glyph::Param::operator=(Glyph::Param& a) {
  name = a.name;
  position = a.position;
  applytosubpath = a.applytosubpath;
  applytopoint = a.applytopoint;
  isEquation = a.isEquation;
  isInControllePath = a.isInControllePath;
  affects = a.affects;
  value = a.value;
  expr = a.expr->clone();
  return *this;
}
Glyph::Param& Glyph::Param::operator=(Glyph::Param&& a) {
  name = a.name;
  position = a.position;
  applytosubpath = a.applytosubpath;
  applytopoint = a.applytopoint;
  isEquation = a.isEquation;
  isInControllePath = a.isInControllePath;
  affects = a.affects;
  value = a.value;
  expr = std::move(a.expr);
  return *this;
}

Glyph::Glyph(QString code, MP mp, Font* parent) : QObject((QObject*)parent) {
  edge = NULL;
  this->mp = mp;
  this->font = parent;
  m_unicode = -1;
  m_charcode = -1;
  m_lefttatweel = 0;
  m_righttatweel = 0;

  this->setSource(code);

  m_undoStack = new QUndoStack(this);
}

Glyph::~Glyph() {

}
void  Glyph::setBeginMacroName(QString macro) {
  m_beginmacroname = macro;
}
QString  Glyph::beginMacroName() {
  return m_beginmacroname;
}

void Glyph::setSource(QString source, bool structureChanged) {

  isSetSource = true;
  blockSignals(true);
  isDirty = true;


  params.clear();
  dependents.clear();

  QList<QByteArray> dynamicProperties = dynamicPropertyNames();
  for (int i = 0; i < dynamicProperties.length(); i++) {
    QByteArray propname = dynamicProperties[i];
    QVariant variant;
    setProperty(propname, variant);
  }

  QHash<Glyph*, ComponentInfo>::iterator comp;
  for (comp = m_components.begin(); comp != m_components.end(); ++comp) {
    Glyph* glyph = comp.key();
    disconnect(glyph, &Glyph::valueChanged, this, &Glyph::componentChanged);
  }

  m_components.clear();
  controlledPaths.clear();

  m_body = "";
  m_verbatim = "";

  glyphparser::Driver driver(*this);

  driver.parse_string(source.toStdString(), name().toStdString());

  edge = NULL;
  isDirty = true;

  m_source = this->source();

  isSetSource = false;

  blockSignals(false);

  //auto gg = receivers(SIGNAL(valueChanged(QString)));

  emit valueChanged("source", structureChanged);
}
QString Glyph::source() {

  if (isDirty) {
    edge = NULL;
    QString source;

    //source = QString("%1(%2,%3,%4,%5,%6,%7,%8);\n").arg(beginMacroName()).arg(name()).arg(unicode()).arg(width()).arg(height()).arg(depth()).arg(leftTatweel()).arg(rightTatweel());
    source = QString("%1(%2,%3,%4,%5,%6);\n").arg(beginMacroName()).arg(name()).arg(unicode()).arg(width()).arg(height()).arg(depth());
    //source = source % QString("%%glyphname:%1\n").arg(name());
    if (!image().path.isEmpty()) {
      Glyph::ImageInfo imageInfo = image();
      QTransform transform = imageInfo.transform;
      source = source % QString("%%backgroundimage:%1,%2,%3,%4,%5,%6,%7\n")
        .arg(imageInfo.path)
        .arg(imageInfo.pos.x())
        .arg(imageInfo.pos.y())
        .arg(transform.m11())
        .arg(transform.m12())
        .arg(transform.m21())
        .arg(transform.m22());
    }

    if (!m_components.isEmpty()) {
      source = source % "%%begincomponents\n";
      QHash<Glyph*, ComponentInfo>::iterator i;
      for (i = m_components.begin(); i != m_components.end(); ++i) {
        ComponentInfo component = i.value();
        QTransform transform = component.transform;
        source = source % QString("drawcomponent(%1,%2,%3,%4,%5,%6,%7);\n").arg(i.key()->name())
          .arg(component.pos.x())
          .arg(component.pos.y())
          .arg(transform.m11())
          .arg(transform.m12())
          .arg(transform.m21())
          .arg(transform.m22());
      }
      source = source % "%%endcomponents\n";
    }

    if (params.size() > 0) {
      source = source % "%%beginparams\n";
      QPointF valpoint;
      QString typepoint;
      double valdouble;
      for (const auto& [key, param] : params) {
        QString propname = param.name;
        QString affect = param.isEquation ? "=" : ":=";

        auto expString = param.expr->toString();

        QString comment;

        if (!param.affects.isEmpty()) {
          comment = " % " + param.affects;
        }

        source = source % QString("%1 %2 %3;%4\n").arg(QString(propname)).arg(affect).arg(expString).arg(comment);


      }
      /*
      QMapIterator<QString, Param> i(params);
      while (i.hasNext()) {
        i.next();
        Param param = i.value();
        QString propname = param.name;
        QString affect = param.isEquation ? "=" : ":=";
        switch (param.type) {
        case expression:

          auto value = property(propname.toLatin1());

          auto exp = expressions.value(propname);

          exp->setConstantValue(value);

          auto expString = exp->toString();

          QString comment;

          if (!param.affects.isEmpty()) {
            comment = " % " + param.affects;
          }



          source = source % QString("%1 %2 %3;%4\n").arg(QString(propname)).arg(affect).arg(expString).arg(comment);
          break;
        }
      }*/
    }

    if (verbatim() != "") {
      source = source % "\n%%beginverbatim\n";
      source = source % verbatim();
      source = source % "%%endverbatim\n";
    }

    if (!controlledPaths.isEmpty()) {
      source = source % "\n%%beginpaths\n\n";
      QMapIterator<int, QMap<int, Glyph::Knot*> > j(controlledPaths);
      while (j.hasNext()) {
        j.next();
        QMap<int, Knot*> path = j.value();
        const Knot* firstpoint = path.first();
        const Knot* currentpoint = firstpoint;
        if (controlledPathNames[j.key()] == "fill") {
          source = source % QString("fill\n");
        }
        else if (controlledPathNames[j.key()] == "fillc") {
          source = source % QString("fillc\n");
        }
        else {
          source = source % QString("controlledPath (%1,%2)(%3)(\n").arg(j.key()).arg(path.firstKey()).arg(controlledPathNames[j.key()]);
        }

        QMapIterator<int, Glyph::Knot*> h(path);
        h.next();
        while (h.hasNext()) {
          h.next();
          const Knot* nextpoint;
          nextpoint = h.value();

          source = source % currentpoint->expr->toString();


          if (currentpoint->rightValue.jointtype == path_join_control) {
            source = source % QString(" .. controls %1").arg(currentpoint->rightValue.dirExpr->toString());
            if (!currentpoint->rightValue.isEqualAfter) {
              source = source % QString(" and %1").arg(nextpoint->leftValue.dirExpr->toString());
            }
            source = source % QString(" ..\n");
          }
          else {
            if (currentpoint->rightValue.type == mpgui_given) {
              source = source % QString(" {%1}").arg(currentpoint->rightValue.dirExpr->toString());
            }
            if (currentpoint->rightValue.jointtype == path_join_macro) {
              source = source % QString(" %1\n").arg(currentpoint->rightValue.macrovalue);
            }
            else {
              if (currentpoint->rightValue.tensionExpr) {
                source = source % QString(" .. tension %1%2").arg(currentpoint->rightValue.isAtleast ? "atleast " : "").arg(currentpoint->rightValue.tensionExpr->toString());
                if (!currentpoint->rightValue.isEqualAfter) {
                  if (nextpoint->leftValue.tensionExpr) {
                    source = source % QString(" and %1%2").arg(nextpoint->leftValue.isAtleast ? "atleast " : "").arg(nextpoint->leftValue.tensionExpr->toString());
                  }
                }
              }

              
              source = source % QString(" ..\n");
            }
            if (nextpoint->leftValue.type == mpgui_given) {
              source = source % QString(" {%1}").arg(nextpoint->leftValue.dirExpr->toString());
            }
          }
          currentpoint = h.value();
        }
        source = source % QString(" cycle\n");
        if (controlledPathNames[j.key()] != "fill" && controlledPathNames[j.key()] != "fillc") {
          source = source % QString(");\n");
        }
        else {
          source = source % QString(";\n");
        }

      }
      source = source % "\n%%endpaths\n";
    }


    if (body() != "") {
      source = source % "%%beginbody\n";
      source = source % body();
    }

    if (m_beginmacroname == "beginchar") {
      source = source % "endchar;\n";
    }
    else {
      source = source % "enddefchar;\n";
    }


    m_source = source;
    isDirty = false;
  }
  return m_source;
}
void Glyph::setName(QString name) {

  if (name == m_name)
    return;

  font->glyphperName.remove(name);
  m_name = name;
  isDirty = true;
  font->glyphperName.insert(m_name, this);
  if (m_unicode == -1) {
    m_charcode = -1;
  }
  emit valueChanged("name");
}
QString Glyph::name() const {
  return m_name;
}
void Glyph::setUnicode(int unicode) {
  if (unicode != m_unicode) {
    m_unicode = unicode;
    m_charcode = unicode;
    isDirty = true;
    if (edge) {
      edge->charcode = charcode();
    }
    emit valueChanged("unicode");
  }
}
int Glyph::unicode() const {
  return m_unicode;
}
int Glyph::charcode() {
  if (m_charcode == -1) {
    QString command("show " + name() + ";");

    QByteArray commandBytes = command.toLatin1();
    //mp->history = mp_spotless;
    int status = mp_execute(mp, (char*)commandBytes.constData(), commandBytes.size());
    mp_run_data* results = mp_rundata(mp);
    QString ret(results->term_out.data);
    ret.trimmed();
    if (status == mp_error_message_issued || status == mp_fatal_error_stop) {
      mp_finish(mp);
      throw "Could not get charcode !\n" + ret;
    }

    m_charcode = ret.mid(3).toInt();
  }
  return m_charcode;
}


void Glyph::setWidth(double width) {
  m_width = width;
  isDirty = true;
  emit valueChanged("width");
}
double Glyph::width() const {
  return m_width;
}
void Glyph::setHeight(double height) {
  m_height = height;
  isDirty = true;
  emit valueChanged("height");
}
double Glyph::height() const {
  return m_height;
}
void Glyph::setDepth(double depth) {
  m_depth = depth;
  isDirty = true;
  emit valueChanged("depth");
}
double Glyph::depth() const {
  return m_depth;
}
void Glyph::setleftTatweel(double lefttatweel) {
  m_lefttatweel = lefttatweel;
  isDirty = true;
  emit valueChanged("leftTatweel", true);
}
double Glyph::leftTatweel() const {
  return m_lefttatweel;
}
void Glyph::setrightTatweel(double righttatweel) {
  m_righttatweel = righttatweel;
  isDirty = true;
  emit valueChanged("rightTatweel", true);
}
double Glyph::rightTatweel() const {
  return m_righttatweel;
}
void Glyph::setImage(Glyph::ImageInfo image) {
  ImageInfo old = m_image;
  m_image = image;
  isDirty = true;
  if (old.path != m_image.path) {
    emit valueChanged("image");
  }
  else if (old.pos != m_image.pos | old.transform != m_image.transform) {
    emit valueChanged("imagetransform");
  }
}
Glyph::ImageInfo Glyph::image() const {
  return m_image;
}
void Glyph::setComponents(QHashGlyphComponentInfo components) {

  isDirty = true;

  QHash<Glyph*, ComponentInfo>::iterator comp;
  for (comp = m_components.begin(); comp != m_components.end(); ++comp) {
    Glyph* glyph = comp.key();
    disconnect(glyph, &Glyph::valueChanged, this, &Glyph::componentChanged);
  }

  m_components.clear();

  for (comp = components.begin(); comp != components.end(); ++comp) {
    Glyph* glyph = comp.key();
    connect(glyph, &Glyph::valueChanged, this, &Glyph::componentChanged);
  }

  m_components = components;

  emit valueChanged("components");

}
Glyph::QHashGlyphComponentInfo Glyph::components() const {
  return m_components;
}
void Glyph::setBody(QString body, bool autoParam) {

  /*
        if (autoParam) {
                static int  tempindex = 1;
                int adjust = 0;

                QRegularExpression regpair("\\(\\s*(\\d*\\.?\\d*)\\s*,\\s*(\\d*\\.?\\d*)\\s*\\)", QRegularExpression::DotMatchesEverythingOption);
                QRegularExpressionMatchIterator i = regpair.globalMatch(body);
                while (i.hasNext()) {
                        QRegularExpressionMatch match = i.next();
                        Param param;
                        param.name = QString("tmpp%1").arg(tempindex++);
                        param.type = point;
                        params.append(param);
                        double x = match.captured(1).toDouble();
                        double y = match.captured(2).toDouble();
                        setProperty(param.name.toLatin1(), QPointF(x, y));
                        QString replace = " " + param.name + " ";
                        body.replace(match.capturedStart(0) + adjust, match.capturedLength(0), replace);
                        adjust += replace.length() - match.capturedLength(0);
                }
        }*/

  m_body = body;
  isDirty = true;
  emit valueChanged("body");
}
QString Glyph::body() const {
  return m_body;
}
void Glyph::setVerbatim(QString verbatim) {
  m_verbatim = verbatim;
  isDirty = true;
  emit valueChanged("verbatim");
}
QString Glyph::verbatim()const {
  return m_verbatim;
}
void Glyph::setComponent(QString name, double x, double y, double t1, double t2, double t3, double t4) {
  isDirty = true;

  ComponentInfo component;
  Glyph* glyph = font->glyphperName[name];
  component.pos = QPointF(x, y);
  QTransform transform(t1, t2, t3, t4, 0, 0);
  component.transform = transform;
  m_components[glyph] = component;
  connect(glyph, &Glyph::valueChanged, this, &Glyph::componentChanged);

  emit valueChanged("components");
}
void Glyph::parseComponents(QString componentSource) {

  /*
        isDirty = true;

        QHash<Glyph*, ComponentInfo>::iterator comp;
        for (comp = m_components.begin(); comp != m_components.end(); ++comp) {
                Glyph* glyph = comp.key();
                disconnect(glyph, &Glyph::valueChanged, this, &Glyph::componentChanged);
        }

        m_components.clear();

        QRegularExpression regcomponents("^drawcomponent\\((.*?),(.*?),(.*?),(.*?),(.*?),(.*?),(.*?)\\);");
        regcomponents.setPatternOptions(QRegularExpression::MultilineOption);
        QRegularExpressionMatchIterator i = regcomponents.globalMatch(componentSource);
        while (i.hasNext()) {
                QRegularExpressionMatch match = i.next();
                ComponentInfo component;
                Glyph* glyph = font->glyphperUnicode[match.captured(1).toLong()];
                component.pos = QPointF(match.captured(2).toDouble(), match.captured(3).toDouble());
                QTransform transform(match.captured(4).toDouble(), match.captured(5).toDouble(),
                        match.captured(6).toDouble(), match.captured(7).toDouble(), 0, 0);
                component.transform = transform;
                m_components[glyph] = component;
                connect(glyph, &Glyph::valueChanged, this, &Glyph::componentChanged);
        }

        emit valueChanged("components");*/
}
void Glyph::parsePaths(QString pathsSource) {
  /*
        QRegularExpression regpaths("fill\\s+(.+?)\\s+((?:\\.\\.\\.|\\.\\.|--).*);");
        QRegularExpression regpair("\\(\\s*([+-]?\\d*\\.?\\d*)\\s*,\\s*([+-]?\\d*\\.?\\d*)\\s*\\)");
        //QRegularExpression regcontrols("(?:\\{\\(.*?)\\})?(\\.\\.\\s*controls()\\s\\.\\.)(?:\\{\\(.*?)\\})?(.+?)(?:\\.\\.\\.|\\.\\.|--)?");
        //QRegularExpression regcontrols("(\\.\\.\\s*controls()\\s\\.\\.)(.+?)(?:\\.\\.\\.|\\.\\.|--)?");
        QString direction = "(?:\\s*\\{(.*?)\\}\\s*)?";
        QString controltension = "(?:\\.\\.\\s*(controls|tension)(?:((?:(?!\\.\\.).)*?)\\s*and\\s*(.*?)|(.*?))\\.\\.)";
        QString regcontrolsstr = direction + "(?:" + controltension + "|(--)" + ")" + direction;
        QRegularExpression regcontrols(regcontrolsstr);
        regpaths.setPatternOptions(QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatchIterator i = regpaths.globalMatch(pathsSource);
        int nbpath = 0;
        while (i.hasNext()) {
                QRegularExpressionMatch match = i.next();
                Knot knot;
                QMap<int, Knot> newpath;
                QString value = match.captured(1);
                QRegularExpressionMatch pairmatch = regpair.match(value);
                if (pairmatch.hasMatch()) {
                        newpath[0].isConstant = true;
                        newpath[0].x = pairmatch.captured(1).toDouble();
                        newpath[0].y = pairmatch.captured(2).toDouble();
                }
                else {
                        newpath[0].isConstant = false;
                        newpath[0].value = value;
                }

                regcontrols.setPatternOptions(QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption);
                QRegularExpressionMatchIterator icontrols = regcontrols.globalMatch(match.captured(2));
                while (icontrols.hasNext()) {
                        QRegularExpressionMatch match = icontrols.next();
                }
                controlledPaths[nbpath++] = newpath;

        }

        emit valueChanged("paths");*/
}
void Glyph::componentChanged(QString name, bool structureChanged) {
  emit valueChanged("component", false);
}

void Glyph::setParameter(QString name, MFExpr* exp, bool isEquation) {
  setParameter(name, exp, isEquation, false, QString());
}
void Glyph::setParameter(QString name, MFExpr* exp, bool isEquation, bool isInControllePath) {
  setParameter(name, exp, isEquation, isInControllePath, QString());
}
void Glyph::setParameter(QString name, MFExpr* exp, bool isEquation, bool isInControllePath, QString affects) {
  Param param = {};

  param.name = name;
  param.isEquation = isEquation;
  param.affects = affects;
  param.expr = std::unique_ptr<MFExpr>{ exp };

  param.isInControllePath = isInControllePath;

  if (name.contains("deltas")) {
    param.isInControllePath = true;
  }

  params.insert({ param.name, std::move(param) });

  if (!affects.isEmpty()) {
    dependents.insert(affects, &params[name]);
  }

  //TODO : check this
  if (exp->isLiteral()) {
    setProperty(name.toLatin1(), exp->constantValue(0));
  }

}

QString Glyph::getError()
{
  mp_run_data* results = mp_rundata(mp);
  QString ret(results->term_out.data);
  return ret.trimmed();
}

QString Glyph::getLog() {
  mp_run_data* results = mp_rundata(mp);
  QString ret(results->log_out.data);
  return ret.trimmed();
}

mp_edge_object* Glyph::getEdge(bool resetExpParams)
{
  if (edge && !isDirty)
    return edge;

  auto data = source();
  /*
  if (expressions.size() > 0) {
    QString end = "enddefchar;\n";
    if (m_beginmacroname == "beginchar") {
      end = "endchar;\n";
    }
    QString addExp;
    for (int i = 0; i < expressions.size(); ++i) {
      auto name = expressions.keys().at(i);
      auto value = expressions.value(name);
      if (value->type() == QVariant::PointF) {
        addExp = addExp % QString("tmp_pair_params_%1:=%2;").arg(i).arg(name);
      }
    }
    addExp = addExp % end;
    data.replace(data.size() - end.size(), end.size(), addExp);

  }*/

  if (!resetExpParams) {
    data = QString("ignore_exp_parameters:=1;lefttatweel:=%1;righttatweel:=%2;").arg(leftTatweel()).arg(rightTatweel()) % data;
  }

  QByteArray commandBytes = data.toLatin1();
  mp->history = mp_spotless;
  int status = mp_execute(mp, (char*)commandBytes.constData(), commandBytes.size());
  if (status >= mp_error_message_issued) {
    QString error = getError();
    edge = NULL;
    return edge;
  }
  mp_run_data* _mp_results = mp_rundata(mp);
  mp_edge_object* p = _mp_results->edges;
  while (p) {
    if (p->charcode == charcode()) {
      edge = p;
      break;
    }
    p = p->next;
  }

  QPointF translate;
  if (edge != nullptr) {
    translate = QPointF(edge->xpart, edge->ypart);
  }

  for (auto& [name, param] : params) {
    if (!param.isInControllePath) {
      auto expr = param.expr.get();
      if (expr->type() != QVariant::Double) {
        QString varName = !param.affects.isEmpty() ? param.affects : name;       
        QPointF point;
        if (font->getPairVariable(varName, point)) {
          param.value = point + translate;
        }
      }
    }
  }

  return edge;
}

Glyph::ComputedValues Glyph::getComputedValues() {
  mp_edge_object* mp_edge = getEdge();
  ComputedValues values;

  if (mp_edge) {
    values.charcode = mp_edge->charcode;
    values.width = mp_edge->width;
    values.height = mp_edge->height;
    values.depth = mp_edge->depth;
    values.bbox.llx = mp_edge->minx;
    values.bbox.urx = mp_edge->maxx;
    values.bbox.lly = mp_edge->miny;
    values.bbox.ury = mp_edge->maxy;

    if (!std::isnan(mp_edge->xleftanchor)) {
      values.leftAnchor = QPoint(mp_edge->xleftanchor, mp_edge->yleftanchor);
    }
    if (!std::isnan(mp_edge->xrightanchor)) {
      values.rightAnchor = QPoint(mp_edge->xrightanchor, mp_edge->yrightanchor);
    }
  }


  return values;
}

bool Glyph::event(QEvent* e) {
  if (e->type() == QEvent::DynamicPropertyChange) {
    QDynamicPropertyChangeEvent* pe = static_cast<QDynamicPropertyChangeEvent*>(e);
    if (!isSetSource) {
      isDirty = true;
      emit valueChanged(pe->propertyName());
    }
  }
  return QObject::event(e); // don't forget this
}
QUndoStack* Glyph::undoStack() const
{
  return m_undoStack;
}
void Glyph::setPropertyWithUndo(const QString& name, const QVariant& value) {
  ChangeGlyphPropertyCommand* command = new ChangeGlyphPropertyCommand(this, name, value);
  m_undoStack->push(command);

}
QPainterPath Glyph::getPathFromEdge(mp_edge_object* h) {

  QPainterPath localpath;


  if (h) {
    mp_graphic_object* body = h->body;


    if (body) {

      do {
        switch (body->type)
        {
        case mp_fill_code: {
          QPainterPath subpath = mp_dump_solved_path(((mp_fill_object*)body)->path_p);
          localpath.addPath(subpath);

          break;
        }
        default:
          break;
        }

      } while (body = body->next);
    }
  }

  return localpath;
}
QPainterPath Glyph::mp_dump_solved_path(mp_gr_knot h) {
  mp_gr_knot p, q;
  QPainterPath path;
  //path.setFillRule(Qt::OddEvenFill);
  if (h == NULL) return path;

  path.moveTo(h->x_coord, h->y_coord);
  p = h;
  do {
    q = p->next;
    path.cubicTo(p->right_x, p->right_y, q->left_x, q->left_y, q->x_coord, q->y_coord);

    p = q;
  } while (p != h);
  if (h->data.types.left_type != mp_endpoint)
    path.closeSubpath();

  return path;
}
QPainterPath Glyph::getPath() {
  QPainterPath localpath;
  QHash<Glyph*, Glyph::ComponentInfo>::iterator comp;
  for (comp = m_components.begin(); comp != m_components.end(); ++comp) {
    Glyph* compglyph = comp.key();
    QPainterPath compath = compglyph->getPath();
    compath.translate(comp.value().pos);
    compath = comp.value().transform.map(compath);
    localpath.addPath(compath);
  }

  localpath.addPath(getPathFromEdge(getEdge()));

  return localpath;
}

bool Glyph::setProperty(const char* name, const QVariant& value, bool updateParam) {
  if (updateParam) {
    auto param = this->params.find(name);
    if (param != this->params.end()) {
      auto expr = param->second.expr.get();
      if (expr->isConstant(0)) {
        expr->setConstantValue(0, value);
      }
    }
  }
  return QObject::setProperty(name, value);
}
