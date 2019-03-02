/* 
 * Clipper Ruby Bindings
 * Copyright 2010 Mike Owens <http://mike.filespanker.com/>
 *
 * Released under the same terms as Clipper.
 *
 */

#include <clipper.hpp>
#include <ruby.h>

#ifndef DBL2NUM
# define DBL2NUM rb_float_new
#endif

using namespace std;
using namespace ClipperLib;

/**
 * clip types
 * PolyFillTypes
 * http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/ClipType.htm
 */
static ID id_even_odd;
static ID id_non_zero;
static ID id_positive;
static ID id_negative;

/**
 * end types
 * http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/EndType.htm
 */
static ID id_et_closed_polygon;
static ID id_et_closed_line;
static ID id_et_open_square;
static ID id_et_open_round;
static ID id_et_open_butt;

/**
 * join types
 * http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/JoinType.htm
 */
static ID id_jt_square;
static ID id_jt_round;
static ID id_jt_miter;

/**
 * init options
 * http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Classes/Clipper/Methods/Constructor.htm
 */ 
// http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Classes/Clipper/Properties/ReverseSolution.htm
static ID id_init_reverse_solution;
// http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Classes/Clipper/Properties/StrictlySimple.htm
static ID id_init_strictly_simple;
// http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Classes/Clipper/Properties/PreserveCollinear.htm
static ID id_init_preserve_collinear;

static inline Clipper*
XCLIPPER(VALUE x)
{
  Clipper* clipper;
  Data_Get_Struct(x, Clipper, clipper);
  return clipper;
}

static inline PolyFillType
sym_to_filltype(VALUE sym)
{
  ID inp = rb_to_id(sym);

  if (inp == id_even_odd) {
    return pftEvenOdd;
  } else if (inp == id_non_zero) {
    return pftNonZero;
  } else if (inp == id_positive) {
    return pftPositive;
  } else if (inp == id_negative) {
    return pftNegative;
  }

  rb_raise(rb_eArgError, "%s", "Expected fill types: even_odd, non_zero, positive, negative");
}

static inline EndType
sym_to_endtype(VALUE sym)
{
  ID inp = rb_to_id(sym);

  if (inp == id_et_closed_polygon) {
    return etClosedPolygon;
  } else if (inp == id_et_closed_line) {
    return etClosedLine;
  } else if (inp == id_et_open_square) {
    return etOpenSquare;
  } else if (inp == id_et_open_round) {
    return etOpenRound;
  } else if (inp == id_et_open_butt) {
    return etOpenButt;
  }

  rb_raise(rb_eArgError, "%s", "Expected end types: closed_polygon, closed_line, open_square, open_round, open_butt");
}

static inline JoinType
sym_to_jointype(VALUE sym)
{
  ID inp = rb_to_id(sym);

  if (inp == id_jt_square) {
    return jtSquare;
  } else if (inp == id_jt_round) {
    return jtRound;
  } else if (inp == id_jt_miter) {
    return jtMiter;
  }

  rb_raise(rb_eArgError, "%s", "Expected join types: square, round, milter");
}

static inline InitOptions
sym_to_initoptions(VALUE sym)
{
  ID inp = rb_to_id(sym);

  if (inp == id_init_preserve_collinear) {
    return ioPreserveCollinear;
  } else if (inp == id_init_reverse_solution) {
    return ioReverseSolution;
  } else if (inp == id_init_strictly_simple) {
    return ioStrictlySimple;
  }

  rb_raise(rb_eArgError, "%s", "Expected init options: preserve_collinear, reverse_solution, strictly_simple");
}

extern "C" {

static inline IntPoint
xy_to_intpoint(VALUE px, VALUE py, double multiplier) {
  return IntPoint((long64)(NUM2DBL(px) * multiplier), (long64)(NUM2DBL(py) * multiplier));
}

static void
ary_to_path(VALUE ary, Path* poly, double multiplier)
{
  const char* earg =
    "Paths have format: [[p0_x, p0_y], [p1_x, p1_y], ...]";

  Check_Type(ary, T_ARRAY);

  for(long i = 0; i != RARRAY_LEN(ary); i++) {
    VALUE sub = rb_ary_entry(ary, i);
    Check_Type(sub, T_ARRAY);

    if(RARRAY_LEN(sub) != 2) {
      rb_raise(rb_eArgError, "%s", earg);
    }

    VALUE px = rb_ary_entry(sub, 0);
    VALUE py = rb_ary_entry(sub, 1);
    poly->push_back(xy_to_intpoint(px, py, multiplier));
  }
}

static void
ary_to_paths(VALUE ary, Paths* paths, double multiplier)
{
  Check_Type(ary, T_ARRAY);
  for(long i = 0; i != RARRAY_LEN(ary); i++) {
    Path p;
    VALUE sub = rb_ary_entry(ary, i);
    Check_Type(sub, T_ARRAY);
    ary_to_path(sub, &p, multiplier);
    paths->push_back(p);
  }
}

static void
rbclipper_free(void* ptr)
{
  delete (Clipper*) ptr;
}

static VALUE
rbclipper_new(VALUE klass)
{
  Clipper* ptr = new Clipper;
  VALUE r = Data_Wrap_Struct(klass, 0, rbclipper_free, ptr);
  rb_obj_call_init(r, 0, 0);
  return r;
}

static VALUE
rbclipper_initialize(int argc, VALUE* argv, VALUE self)
{
  VALUE multiplier;

  if (argc == 1) {
    multiplier = argv[0];
  } else if (argc == 0) {
    multiplier = INT2NUM(1048576); // 2 ^ 10
  } else {
    rb_raise(rb_eArgError, "wrong number of arguments");
  }

  rb_iv_set(self, "@multiplier", multiplier);
  return self;
}

static VALUE
rbclipper_add_polygon_internal(VALUE self, VALUE polygon,
                               PolyType polytype)
{
  double multiplier = NUM2DBL(rb_iv_get(self, "@multiplier"));
  Path tmp;
  ary_to_path(polygon, &tmp, multiplier);
  XCLIPPER(self)->AddPath(tmp, polytype, true);
  return Qnil;
}

static VALUE
rbclipper_add_subject_polygon(VALUE self, VALUE polygon)
{
  return rbclipper_add_polygon_internal(self, polygon, ptSubject);
}

static VALUE
rbclipper_add_clip_polygon(VALUE self, VALUE polygon)
{
  return rbclipper_add_polygon_internal(self, polygon, ptClip);
}

static VALUE
rbclipper_add_polygons_internal(VALUE self, VALUE polygons,
                                    PolyType polytype)
{
  double multiplier = NUM2DBL(rb_iv_get(self, "@multiplier"));
  Paths tmp;
  ary_to_paths(polygons, &tmp, multiplier);
  XCLIPPER(self)->AddPaths(tmp, polytype, true);
  return Qnil;
}

static VALUE
rbclipper_add_subject_polygons(VALUE self, VALUE polygons)
{
  return rbclipper_add_polygons_internal(self, polygons, ptSubject);
}

static VALUE
rbclipper_add_clip_polygons(VALUE self, VALUE polygons)
{
  return rbclipper_add_polygons_internal(self, polygons, ptClip);
}

static VALUE
rbclipper_clear(VALUE self)
{
  XCLIPPER(self)->Clear();
  return Qnil;
}

static VALUE
rbclipper_execute_internal(VALUE self, ClipType cliptype,
                           VALUE subjfill, VALUE clipfill)
{
  if (NIL_P(subjfill))
    subjfill = ID2SYM(id_even_odd);

  if (NIL_P(clipfill))
    clipfill = ID2SYM(id_even_odd);

  double inv_multiplier = 1.0 / NUM2LONG(rb_iv_get(self, "@multiplier"));

  Paths solution;
  XCLIPPER(self)->Execute((ClipType) cliptype,
                          solution,
                          sym_to_filltype(subjfill),
                          sym_to_filltype(clipfill));
  VALUE r = rb_ary_new();
  for(Paths::iterator i = solution.begin(); i != solution.end(); ++i) {
    VALUE sub = rb_ary_new();
    for(Path::iterator p = i->begin(); p != i->end(); ++p) {
      rb_ary_push(sub, rb_ary_new3(2, DBL2NUM(p->X * inv_multiplier), DBL2NUM(p->Y * inv_multiplier)));
    }
    rb_ary_push(r, sub);
  }

  return r;
}

static VALUE
rbclipper_intersection(int argc, VALUE* argv, VALUE self)
{
  VALUE subjfill, clipfill;
  rb_scan_args(argc, argv, "02", &subjfill, &clipfill);
  return rbclipper_execute_internal(self, ctIntersection, subjfill, clipfill);
}

static VALUE
rbclipper_union(int argc, VALUE* argv, VALUE self)
{
  VALUE subjfill, clipfill;

  rb_scan_args(argc, argv, "02", &subjfill, &clipfill);

  return rbclipper_execute_internal(self, ctUnion, subjfill, clipfill);
}

static VALUE
rbclipper_difference(int argc, VALUE* argv, VALUE self)
{
  VALUE subjfill, clipfill;
  rb_scan_args(argc, argv, "02", &subjfill, &clipfill);
  return rbclipper_execute_internal(self, ctDifference, subjfill, clipfill);
}

static VALUE
rbclipper_xor(int argc, VALUE* argv, VALUE self)
{
  VALUE subjfill, clipfill;
  rb_scan_args(argc, argv, "02", &subjfill, &clipfill);
  return rbclipper_execute_internal(self, ctXor, subjfill, clipfill);
}

typedef VALUE (*ruby_method)(...);

void Init_clipper() {
  // fill types
  id_even_odd = rb_intern("even_odd");
  id_non_zero = rb_intern("non_zero");
  id_positive = rb_intern("positive");
  id_negative = rb_intern("negative");
  // end types
  id_et_closed_polygon  = rb_intern("closed_polygon");
  id_et_closed_line     = rb_intern("closed_line");
  id_et_open_square     = rb_intern("open_square");
  id_et_open_round      = rb_intern("open_round");
  id_et_open_butt       = rb_intern("open_butt");
  // join types
  id_jt_square  = rb_intern("square");
  id_jt_round   = rb_intern("round");
  id_jt_miter   = rb_intern("miter");
  // init options
  id_init_reverse_solution    = rb_intern("reverse_solution");
  id_init_strictly_simple     = rb_intern("strictly_simple");
  id_init_preserve_collinear  = rb_intern("preserve_collinear");

  VALUE mod = rb_define_module("Clipper");
  VALUE k   = rb_define_class_under(mod, "Clipper", rb_cObject);

  rb_define_singleton_method(k, "new", (ruby_method) rbclipper_new, 0);
  rb_define_method(k, "initialize",(ruby_method) rbclipper_initialize, -1);
  rb_define_attr(k, "multiplier", 1, 0);

  rb_define_method(k, "add_subject_polygon",
                   (ruby_method) rbclipper_add_subject_polygon, 1);
  rb_define_method(k, "add_clip_polygon",
                   (ruby_method) rbclipper_add_clip_polygon, 1);

  rb_define_method(k, "add_subject_polygons",
                   (ruby_method) rbclipper_add_subject_polygons, 1);
  rb_define_method(k, "add_clip_polygons",
                   (ruby_method) rbclipper_add_clip_polygons, 1);

  rb_define_method(k, "clear!",
                   (ruby_method) rbclipper_clear, 0);

  rb_define_method(k, "intersection",
                   (ruby_method) rbclipper_intersection, -1);
  rb_define_method(k, "union",
                   (ruby_method) rbclipper_union, -1);
  rb_define_method(k, "difference",
                   (ruby_method) rbclipper_difference, -1);
  rb_define_method(k, "xor",
                   (ruby_method) rbclipper_xor, -1);
}

} /* extern "C" */
