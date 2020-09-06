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

#ifndef H_FEAAST
# define H_FEAAST

#include <map>
#include <vector>
#include <ostream>
#include <stdexcept>
#include <cmath>
#include <qstack.h>

#include "Lookup.h"
#include "Subtable.h"
#include "OtLayout.h"
#include <qdebug.h>

class QJsonObject;

namespace feayy {

	class Visitor;


	class Statement {
	public:
		virtual void accept(Visitor&) = 0;
		virtual ~Statement() {}
	};

	class LookupStatement : public Statement {
	public:
		void accept(Visitor&) override;
	};



	enum class GlyphType {
		CID,
		Name
	};

	enum class AnchorType {
		Null,
		Name,
		FormatA,
		Function
	};

	enum class InlineType {
		None,
		SinglePos,
		CursivePos
	};

	enum class ClassComponentType {
		Glyph,
		ClassName,
		RegExp
	};

	class ClassComponent {
		ClassComponentType _componenttype;
	public:
		explicit ClassComponent(ClassComponentType type) :_componenttype{ type } {};

		ClassComponentType componentType() {
			return _componenttype;
		}

		virtual ~ClassComponent()
		{
		}

		virtual QSet<quint16> getCodes(OtLayout* otlayout) = 0;
	};



	class Glyph : public ClassComponent {
		GlyphType _glyphtype;
	public:
		explicit Glyph(GlyphType type) :ClassComponent{ ClassComponentType::Glyph }, _glyphtype{ type } {};

		GlyphType glyphType() {
			return _glyphtype;
		}
	};

	class Anchor {
		AnchorType _anchortype;
	public:
		explicit Anchor(AnchorType type) :_anchortype{ type } {};

		AnchorType anchortype() {
			return _anchortype;
		}
	};

	class AnchorNull : public Anchor {
	public:
		explicit AnchorNull() :Anchor(AnchorType::Null) {}

		~AnchorNull() {
		}
	};


	class AnchorName : public Anchor {

	public:

        std::string* name;

		explicit AnchorName(std::string* _name) :Anchor(AnchorType::Name), name{ _name } {}

		operator std::string* () const { return name; }

		~AnchorName() {
			delete name;
		}
	};

	class AnchorFunction : public Anchor {
	public:

		std::string* name;

		explicit AnchorFunction(std::string* _name) :Anchor(AnchorType::Function), name{ _name } {}

		operator std::string* () const { return name; }

		~AnchorFunction() {
			delete name;
		}
	};

	class AnchorFormatA : public Anchor {

	public:
		int x;
		int y;

		explicit AnchorFormatA(int x, int y) :Anchor{ AnchorType::FormatA }, x{ x }, y{ y } {}

		~AnchorFormatA() {
		}
	};



	class GlyphName : public Glyph {
		std::string* name;
	public:
		explicit GlyphName(std::string* _name) :Glyph(GlyphType::Name), name{ _name } {}

		operator std::string* () const { return name; }

		~GlyphName() {
			delete name;
		}

		QSet<quint16> getCodes(OtLayout* otlayout) {
			QString lname = QString::fromStdString(*name);
			if (otlayout->glyphCodePerName.contains(lname)) {
				return { otlayout->glyphCodePerName[lname] };
			}
			else {
				qDebug() << "Glyph Name " + lname + " not found";
				//return { otlayout->glyphCodePerName[lname] };
				throw "Glyph Name " + lname + " not found";
			}
		}

	};

	class ClassName : public ClassComponent {
		std::string* name;
	public:
		explicit ClassName(std::string* _name) :ClassComponent(ClassComponentType::ClassName), name{ _name } {}

		operator std::string* () const { return name; }

		~ClassName() {
			delete name;
		}

		QSet<quint16> getCodes(OtLayout* otlayout) {
			QString lname = QString::fromStdString(*name);
			auto set = otlayout->classtoUnicode(lname);
			if (set.isEmpty()) {
				throw "Class " + lname + " is empty";
			}
			return set;
		}

	};

	class RegExpClass : public ClassComponent {
		std::string* _regexpr;
	public:
		explicit RegExpClass(std::string* _reg) :ClassComponent(ClassComponentType::RegExp), _regexpr{ _reg } {}

		operator std::string* () const { return _regexpr; }

		~RegExpClass() {
			delete _regexpr;
		}

		QSet<quint16> getCodes(OtLayout* otlayout) {
			QString regexp = QString::fromStdString(*_regexpr);
			auto set = otlayout->regexptoUnicode(regexp);
			return set;
		}

	};

	class GlyphCID : public Glyph {
		int cid;
	public:
		explicit GlyphCID(int _cid) :Glyph(GlyphType::CID), cid{ _cid } {}

		operator int() const { return cid; }

		QSet<quint16> getCodes(OtLayout* otlayout) {
			if (otlayout->glyphNamePerCode.contains(cid)) {
				return { (quint16)cid };
			}
			else {
				if (cid != char(10)) {
					//qDebug() << QString("Glyph code %1 not found").arg(cid);
					//return { otlayout->glyphCodePerName[lname] };
					return { (quint16)cid };
				}
				else {
					return { (quint16)10 };
				}

			}
		}
	};

	class GlyphClass {
		std::vector<ClassComponent*>* _components;
	public:
		explicit GlyphClass(std::vector<ClassComponent*>* components) : _components{ components } {
		}

		explicit GlyphClass(ClassName* component) {
			_components = new std::vector<ClassComponent*>{ component };
		}

		explicit GlyphClass(RegExpClass* component) {
			_components = new std::vector<ClassComponent*>{ component };
		}

		virtual ~GlyphClass()
		{
			for (auto component : *_components) {
				delete component;
			}
			delete _components;
		}

		QSet<quint16> getCodes(OtLayout* otlayout) {

			QSet<quint16> unicodes;
			for (auto comp : *_components) {
				unicodes.unite(comp->getCodes(otlayout));
			}

			return unicodes;
		}
	};

	class GlyphSet {
		enum class Tag { glyphclass, glyph, empty };

		union {
			GlyphClass* _glyphClass;
			Glyph* _glyph;
		};
		Tag type;
	public:
		struct Bad_entry { }; // used for exceptions
		explicit GlyphSet(GlyphClass* _glyphClass) :_glyphClass(_glyphClass), type{ Tag::glyphclass } {}
		explicit GlyphSet(Glyph* _glyph) :_glyph(_glyph), type{ Tag::glyph } {}
		explicit GlyphSet() :_glyph(nullptr), type{ Tag::empty } {}

		GlyphClass* glyphClass() const {
			if (type != Tag::glyphclass) throw Bad_entry{};
			return _glyphClass;
		}

		Glyph* glyph() const {
			if (type != Tag::glyph) throw Bad_entry{};
			return _glyph;
		}

		~GlyphSet() {
			if (type == Tag::glyphclass) {
				delete _glyphClass;
			}
			else if (type == Tag::glyph) {
				delete _glyph;
			}
		}

		QSet<quint16> getCodes(OtLayout* otlayout) {

			if (type == Tag::glyphclass) {
				return _glyphClass->getCodes(otlayout);
			}
			else if (type == Tag::glyph) {
				return _glyph->getCodes(otlayout);
			}
			else {
				return QSet<quint16>();
			}
		}

		QList<quint16> getSortedCodes(OtLayout* otlayout) {

			auto set = getCodes(otlayout);
			auto list = set.values();
			std::sort(list.begin(), list.end());
			//qSort(list);
			return list;
		}

		bool isEmpty() {
			return type == Tag::empty;
		}

	};

	class Mark2BaseClass {
	public:
		GlyphSet* glyphset;
		Anchor* baseAnchor;
		Anchor* markAnchor;
		std::string* className;

		explicit Mark2BaseClass(GlyphSet* glyphset, Anchor* baseAnchor, Anchor* markAnchor, std::string* className)
			:glyphset{ glyphset }, baseAnchor{ baseAnchor }, markAnchor{ markAnchor }, className{ className } {}

		~Mark2BaseClass() {
			delete glyphset;
			delete baseAnchor;
			delete markAnchor;
			delete className;
		}
	};



	class GlyphSetRegExp {
	public:
		virtual ~GlyphSetRegExp()
		{
		}

		virtual  QVector<QVector<GlyphSet*>> getSequences() = 0;
	};

	class GlyphSetRegExpSingle : public GlyphSetRegExp {
	public:

		GlyphSet* element;

		explicit GlyphSetRegExpSingle(GlyphSet* element) : element{ element } {}

		virtual ~GlyphSetRegExpSingle()
		{
			delete element;
		}

		QVector<QVector<GlyphSet*>> getSequences() override {
			if (element->isEmpty()) {
				return { {} };
			}
			else {
				return { { element } };
			}
		}

	};

	class GlyphSetRegExpGlyphSeq : public GlyphSetRegExp {
	public:

		QVector<GlyphSet*> seq;

		explicit GlyphSetRegExpGlyphSeq(vector<Glyph *>*  glyphSeq){
			for(auto glyph : *glyphSeq){
				this->seq.append(new GlyphSet(glyph));
			}
			delete glyphSeq;
		}

		virtual ~GlyphSetRegExpGlyphSeq()
		{
			for(auto glyph : seq){
				delete glyph;
			}
		}

		QVector<QVector<GlyphSet*>> getSequences() override {
			if (seq.isEmpty()) {
				return { {} };
			}
			else {
				return { seq };
			}
		}

	};




	class GlyphSetRegExpSeq : public GlyphSetRegExp {
	public:

		GlyphSetRegExp* left;
		GlyphSetRegExp* right;

		explicit GlyphSetRegExpSeq(GlyphSetRegExp* left, GlyphSetRegExp* right) : left{ left }, right{ right } {}
		explicit GlyphSetRegExpSeq(GlyphSetRegExp* left, GlyphSet* right) :left{ left }, right{ new GlyphSetRegExpSingle(right) } {}
		explicit GlyphSetRegExpSeq(GlyphSet* left, GlyphSetRegExp* right) :left{ new GlyphSetRegExpSingle(left) }, right{ right } {}
		explicit GlyphSetRegExpSeq(GlyphSet* left, GlyphSet* right) :left{ new GlyphSetRegExpSingle(left) }, right{ new GlyphSetRegExpSingle(right)  } {}

		virtual ~GlyphSetRegExpSeq()
		{
			delete left;
			delete right;
		}

		QVector<QVector<GlyphSet*>> getSequences() override {
			auto lefseqs = left->getSequences();
			auto rightseqs = right->getSequences();
			QVector<QVector<GlyphSet*>> ret;
			for (auto leftseq : lefseqs) {
				QVector<GlyphSet*> newseq = leftseq;
				for (auto rightseq : rightseqs) {
					QVector<GlyphSet*> newseq{ leftseq };
					newseq.append(rightseq);
					ret.append(newseq);
				}
			}

			return ret;
		}
	};

	class GlyphSetRegExpOr : public GlyphSetRegExp {
	public:

		GlyphSetRegExp* left;
		GlyphSetRegExp* right;

		explicit GlyphSetRegExpOr(GlyphSetRegExp* left, GlyphSetRegExp* right) : left{ left }, right{ right } {}

		virtual ~GlyphSetRegExpOr()
		{
			delete left;
			delete right;
		}

		QVector<QVector<GlyphSet*>> getSequences() override {

			QVector<QVector<GlyphSet*>> ret;

			auto lefseqs = left->getSequences();
			auto rightseqs = right->getSequences();

			for (auto leftseq : lefseqs) {
				ret.append(leftseq);
			}

			for (auto rightseq : rightseqs) {
				ret.append(rightseq);
			}

			return ret;
		}

	};

	class SingleAdjustmentRule : public LookupStatement {
	public:

		GlyphSet* glyphset;
		ValueRecord valueRecord;
		bool color;

		explicit SingleAdjustmentRule(GlyphSet* glyphset, ValueRecord valueRecord, bool color = false)
			:glyphset{ glyphset }, valueRecord{ valueRecord }, color{ color } {}

		void accept(Visitor&) override;
	};


	class CursiveRule : public LookupStatement {
	public:

		GlyphSet* glyphset;
		Anchor* entryAnchor;
		Anchor* exitAnchor;

		explicit CursiveRule(GlyphSet* glyphset, Anchor* entryAnchor, Anchor* exitAnchor)
			:glyphset{ glyphset }, entryAnchor{ entryAnchor }, exitAnchor{ exitAnchor } {}

		void accept(Visitor&) override;

		~CursiveRule() {
			delete glyphset;
			delete entryAnchor;
			delete exitAnchor;
		}
	};

	class Mark2BaseRule : public LookupStatement {
	public:
		GlyphSet* baseGlyphSet;
		vector<Mark2BaseClass*>* mark2baseclasses;
		Lookup::Type type;

		explicit Mark2BaseRule(GlyphSet* baseGlyphSet, vector<Mark2BaseClass*>* mark2baseclasses, Lookup::Type type)
			:baseGlyphSet{ baseGlyphSet }, mark2baseclasses{ mark2baseclasses }, type{ type } {}

		void accept(Visitor&) override;

		virtual ~Mark2BaseRule() {
			delete baseGlyphSet;
			for (auto mark2baseclass : *mark2baseclasses) {
				delete mark2baseclass;
			}
			delete mark2baseclasses;
		}
	};

	class MarkedGlyphSetRegExp {
	public:
		GlyphSetRegExp* regexp = nullptr;
		std::string* lookupName = nullptr;
		InlineType inlineType = InlineType::None;
		LookupStatement* stmt = nullptr;

		virtual void accept(Visitor&);

		explicit MarkedGlyphSetRegExp(GlyphSet* glyphset, std::string* lookupName) :regexp{ new GlyphSetRegExpSingle{glyphset} }, lookupName{ lookupName } {}
		explicit MarkedGlyphSetRegExp(GlyphSet* glyphset, SingleAdjustmentRule* rule) : regexp{ new GlyphSetRegExpSingle{ glyphset } }, inlineType{ InlineType::SinglePos }, stmt{ rule } { };
		explicit MarkedGlyphSetRegExp(GlyphSet* glyphset, CursiveRule* rule) : regexp{ new GlyphSetRegExpSingle{ glyphset } }, inlineType{ InlineType::CursivePos }, stmt{ rule } { };
		explicit MarkedGlyphSetRegExp(GlyphSet* glyphset, ValueRecord valuerecord) : MarkedGlyphSetRegExp{ glyphset, new SingleAdjustmentRule(glyphset, valuerecord, false) } {};

		explicit MarkedGlyphSetRegExp(GlyphSetRegExp* regexp, std::string* lookupName)
			:regexp{ regexp }, lookupName{ lookupName } {}

		~MarkedGlyphSetRegExp() {
			delete regexp;
			delete lookupName;
			//delete stmt;
		}
	};

	class ClassDefinition : public LookupStatement {
	public:
		QString name;
		GlyphClass* components;

		explicit ClassDefinition(std::string* name, GlyphClass* components)
			:name{ QString::fromStdString(*name) }, components{ components } {}

		void accept(Visitor&) override;

		~ClassDefinition() {
			delete components;
		}
	};



	class FeatureReference : public LookupStatement {
	public:
		QString featureName;

		explicit FeatureReference(std::string* name)
			:featureName{ QString::fromStdString(*name) } {}

		void accept(Visitor&) override;
	};

	class LookupReference : public LookupStatement {
	public:
		QString lookupName;

		explicit LookupReference(std::string* name)
			:lookupName{ QString::fromStdString(*name) } {}

		void accept(Visitor&) override;

	};

	class SingleSubstituionRule : public LookupStatement {
	public:

		enum class FirstType {
			GLYPH,
			GLYPHSET
		};

		union {
			Glyph* firstglyph;
			GlyphSet* firstGlyphSet;
		};

		FirstType firstType = FirstType::GLYPH;

		Glyph* secondglyph;
		int format;
		GlyphExpansion expansion;
		StartEndLig startEndLig = StartEndLig::StartEnd;


		explicit SingleSubstituionRule(Glyph* firstglyph, Glyph* secondglyph, int format)
			:SingleSubstituionRule{ firstglyph ,secondglyph ,format ,{},StartEndLig::StartEnd } {}

		explicit SingleSubstituionRule(Glyph* firstglyph, Glyph* secondglyph, int format, GlyphExpansion expansion, StartEndLig startEndLig)
			:firstglyph{ firstglyph }, secondglyph{ secondglyph }, format{ format }, expansion{ expansion }, startEndLig{ startEndLig }{}

		explicit SingleSubstituionRule(Glyph* firstglyph, float lefttatweel, float righttatweel)
			:firstglyph{ firstglyph },secondglyph{nullptr}, format{ 11 }, expansion{ lefttatweel,lefttatweel,righttatweel,righttatweel }, startEndLig{ StartEndLig::StartEnd }{}

		explicit SingleSubstituionRule(GlyphSet* firstGlyphSet, float lefttatweel, float righttatweel)
			:firstGlyphSet{ firstGlyphSet }, firstType{ FirstType::GLYPHSET },secondglyph{ nullptr }, format{ 11 }, expansion{ lefttatweel,lefttatweel,righttatweel,righttatweel },
			startEndLig{ StartEndLig::StartEnd }{}

		void accept(Visitor&) override;

		~SingleSubstituionRule() {

			bool firtequalsecond = firstglyph == secondglyph;

			if (firstType == FirstType::GLYPH) {
				delete firstglyph;
			}
			else {
				delete firstGlyphSet;
			}

			if (!firtequalsecond) {
				delete secondglyph;
			}
			
		}
	};

	class LigatureSubstitutionRule : public LookupStatement {
	public:
		std::vector<Glyph*>* sequence;
		Glyph* ligature;
		int format;

		explicit LigatureSubstitutionRule(std::vector<Glyph*>* sequence, Glyph* ligature)
			:sequence{ sequence }, ligature{ ligature }, format{ 1 }{}

		~LigatureSubstitutionRule() {
			for (auto glyph : *sequence) {
				delete glyph;
			}
			delete ligature;
			delete sequence;
		}

		void accept(Visitor&) override;
	};

	class ChainingContextualRule : public LookupStatement {
	public:
		std::vector<MarkedGlyphSetRegExp*>* input;
		GlyphSetRegExp* backtrack;
		GlyphSetRegExp* lookahead;
		Lookup::Type type;
	public:
		explicit ChainingContextualRule(GlyphSetRegExp* backtrack, std::vector<MarkedGlyphSetRegExp*>* input, GlyphSetRegExp* lookahead)
			:input{ input }, backtrack{ backtrack }, lookahead{ lookahead }, type{ Lookup::none } {}

		~ChainingContextualRule() {
			for (auto glypset : *input) {
				delete glypset;
			}
			delete input;
			delete backtrack;
			delete lookahead;
		}

		void setType(bool isgsub) {
			if (isgsub)
				type = Lookup::chainingsub;
			else
				type = Lookup::chainingpos;
		}

		void accept(Visitor&) override;


	};



	class LookupFlag : public LookupStatement {
	public:

		enum Flags {
			RightToLeft = 0x0001u,
			IgnoreBaseGlyphs = 0x0002u,
			IgnoreLigatures = 0x0004u,
			IgnoreMarks = 0x0008u,
			IgnoreFlags = 0x000Eu,
			UseMarkFilteringSet = 0x0010u,
			Reserved = 0x00E0u,
			MarkAttachmentType = 0xFF00u
		};
		explicit LookupFlag(short flag) :markFilteringSet{ nullptr },flag{ flag } {
		}

		explicit LookupFlag() :markFilteringSet{ nullptr },flag(0) {
		}

		explicit LookupFlag(GlyphSet* set) :markFilteringSet(set),flag(UseMarkFilteringSet) {
		}

		void set_Flag(Flags pflag) {
			flag |= pflag;
		}

		void set_markFilteringSet(GlyphSet* pmarkFilteringSet) {
			if (markFilteringSet != nullptr) {
				delete markFilteringSet;
			}

			flag |= Flags::UseMarkFilteringSet;

			markFilteringSet = pmarkFilteringSet;

		}

		void operator|(LookupFlag lookupFlag) {
			flag |= lookupFlag.flag;
			set_markFilteringSet(lookupFlag.markFilteringSet);
		}

		~LookupFlag() {
			if (markFilteringSet != nullptr) {
				delete markFilteringSet;
			}
		}

		short getFlag() {
			return flag;
		}

		GlyphSet* getMarkFilteringSet() {
			return markFilteringSet;
		}

		void accept(Visitor&) override;

	private:
		GlyphSet* markFilteringSet;
		short flag;
	};




	class LookupDefinition : public LookupStatement {
	public:
		LookupDefinition(std::string* name, std::vector<LookupStatement*>* stmtlist)
			: name{ name }, stmts(stmtlist)
		{}

		~LookupDefinition() {
			if (stmts) {
				for (auto stm : *stmts) {
					delete stm;
				}
			}
			delete stmts;
			delete name;
		}

		std::string getName() {
			return *name;
		}

		std::vector<LookupStatement*>& getStmts() {
			return *stmts;
		}

		void accept(Visitor&) override;

	private:
		std::string* name;
		std::vector<LookupStatement*>* stmts;

	};

	class FeaRoot {
	protected:
		std::vector<Statement*>* stmts;
	public:
		FeaRoot(std::vector<Statement*>* stmtlist)
			: stmts(stmtlist)
		{}
		~FeaRoot() {
			for (auto stmt : *stmts) {
				delete stmt;
			}
			delete stmts;
		}
	};

	class FeatureDefenition : public Statement {
	public:
		FeatureDefenition(std::string* name, std::vector<LookupStatement*>* stmtlist)
			: name{ name }, stmts(stmtlist)
		{}

		~FeatureDefenition() {
			if (stmts) {
				for (auto stm : *stmts) {
					delete stm;
				}
			}
			delete stmts;
			delete name;
		}

		std::string getName() {
			return *name;
		}

		std::vector<LookupStatement*>& getStmts() {
			return *stmts;
		}

		void accept(Visitor&) override;

	private:
		std::string* name;
		std::vector<LookupStatement*>* stmts;

	};



	class FeaContext
	{
	public:


		typedef std::map<std::string, LookupDefinition*> lookupmap_type;
		typedef std::map<std::string, FeatureDefenition*> featuremap_type;

		lookupmap_type		lookups;
		featuremap_type		features;
		const QJsonObject* json;

		FeaRoot* root;

		FeaContext(OtLayout* otlayout, const QJsonObject* json) :json{ json }, otlayout{ otlayout } {
			root = nullptr;
		}

		/// free the saved expression trees
		~FeaContext()
		{
			delete root;
		}

		void useLookup(QString lookupName);

		void populateFeatures();

	private:
		OtLayout* otlayout;
	};

	class Visitor {
	public:
		virtual void accept(LookupFlag&) = 0;
		virtual void accept(LookupDefinition&) = 0;
		virtual void accept(ChainingContextualRule&) = 0;
		virtual void accept(SingleAdjustmentRule&) = 0;
		virtual void accept(CursiveRule&) = 0;
		virtual void accept(Mark2BaseRule&) = 0;
		virtual void accept(SingleSubstituionRule&) = 0;
		virtual void accept(LookupStatement&) = 0;
		virtual void accept(FeatureReference&) = 0;
		virtual void accept(FeatureDefenition&) = 0;
		virtual void accept(ClassDefinition&) = 0;
		virtual void accept(MarkedGlyphSetRegExp&) = 0;
		virtual void accept(LookupReference&) = 0;
		virtual void accept(LigatureSubstitutionRule&) = 0;
		


	};

	class LookupDefinitionVisitor : public Visitor {
	public:

		LookupDefinitionVisitor(OtLayout* otlayout, FeaContext& context);
		void accept(LookupFlag&) override;
		void accept(LookupDefinition&) override;
		void accept(ChainingContextualRule&) override;
		void accept(SingleAdjustmentRule&) override;
		void accept(CursiveRule&) override;
		void accept(Mark2BaseRule&) override;
		void accept(SingleSubstituionRule&) override;
		void accept(LookupStatement&) override;
		void accept(FeatureDefenition&) override;
		void accept(FeatureReference&) override;
		void accept(ClassDefinition&)  override;
		void accept(MarkedGlyphSetRegExp&) override;
		void accept(LookupReference&) override;
		void accept(LigatureSubstitutionRule&) override;

		Lookup* lookup;
		std::string currentFeature;

	private:

		OtLayout* otlayout;
		QSet<QString>* refLookups;
		FeaContext& context;

		QStack<std::tuple<Lookup*, QSet<QString>*, int>> lookupStack;
		int nextautolookup = 1;
	};


}

#endif // H_FEAAST
