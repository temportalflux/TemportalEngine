#include "property/Number.hpp"

using namespace properties;

#define DEFINE_PROPERTY_EDITOR_NUM(TYPE_NUM) PropertyResult properties::renderPropertyEditor(const char* id, TYPE_NUM &value, TYPE_NUM const& defaultValue) \
{ \
	return renderNumbers(id, &defaultValue, &value, 1); \
}

DEFINE_PROPERTY_EDITOR_NUM(i8)
DEFINE_PROPERTY_EDITOR_NUM(ui8)
DEFINE_PROPERTY_EDITOR_NUM(i16)
DEFINE_PROPERTY_EDITOR_NUM(ui16)
DEFINE_PROPERTY_EDITOR_NUM(i32)
DEFINE_PROPERTY_EDITOR_NUM(ui32)
DEFINE_PROPERTY_EDITOR_NUM(i64)
DEFINE_PROPERTY_EDITOR_NUM(ui64)
DEFINE_PROPERTY_EDITOR_NUM(f32)
DEFINE_PROPERTY_EDITOR_NUM(f64)

#undef DEFINE_PROPERTY_EDITOR_NUM

// PropertyResult renderPropertyEditor2(const char* id, VAL_TYPE &value, VAL_TYPE const& defaultValue)