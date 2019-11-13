#include "encode.h"
#include "ColorSpace.h"
#include "farver.h"
#include <cctype>

static char hex8[] = "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F606162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7F808182838485868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9FA0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBFC0C1C2C3C4C5C6C7C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDFE0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF";
static char buffer[] = "#000000";
static char buffera[] = "#00000000";

inline int double2int(double d) {
  d += 6755399441055744.0;
  return reinterpret_cast<int&>(d);
}
inline int cap0255(int x) {
  return x < 0 ? 0 : (x > 255 ? 255 : x);
}
inline int hex2int(const int x) {
  if (!std::isxdigit(x)) {
    Rf_error("Invalid hexadecimal digit");
  }
  return (x & 0xf) + (x >> 6) + ((x >> 6) << 3);
}

template <typename From>
SEXP encode_impl(SEXP colour, SEXP alpha, SEXP white) {
  int n_channels = dimension<From>();
  if (Rf_ncols(colour) < n_channels) {
    Rf_error("Colour in this format must contain at least %i columns", n_channels);
  }
  static ColorSpace::Rgb rgb;
  ColorSpace::XyzConverter::SetWhiteReference(REAL(white)[0], REAL(white)[1], REAL(white)[2]);
  int n = Rf_nrows(colour);
  SEXP codes = PROTECT(Rf_allocVector(STRSXP, n));
  bool has_alpha = !Rf_isNull(alpha);
  char alpha1 = '\0';
  char alpha2 = '\0';
  bool alpha_is_int = false;
  bool one_alpha = false;
  char* buf = NULL;
  int* alpha_i = NULL;
  double* alpha_d = NULL;
  if (has_alpha) {
    buf = buffera;
    alpha_is_int = Rf_isInteger(alpha);
    one_alpha = Rf_length(alpha) == 1;
    int first_alpha;
    if (alpha_is_int) {
      alpha_i = INTEGER(alpha);
      first_alpha = alpha_i[0];
      first_alpha = first_alpha == R_NaInt ? 255 : first_alpha;
    } else {
      alpha_d = REAL(alpha);
      if (!R_finite(alpha_d[0])) {
        first_alpha = 255;
      } else {
        first_alpha = double2int(alpha_d[0]);
      }
    }
    first_alpha = cap0255(first_alpha) * 2;
    alpha1 = hex8[first_alpha];
    alpha2 = hex8[first_alpha + 1];
  } else {
    buf = buffer;
  }
  int offset1 = 0;
  int offset2 = offset1 + n;
  int offset3 = offset2 + n;
  int offset4 = offset3 + n;
  
  int* colour_i = NULL;
  double* colour_d = NULL;
  bool colour_is_int = Rf_isInteger(colour);
  int num;
  if (colour_is_int) {
    colour_i = INTEGER(colour);
  } else {
    colour_d = REAL(colour);
  }
  for (int i = 0; i < n; ++i) {
    if (colour_is_int) {
      fill_rgb<From>(&rgb, colour_i[offset1 + i], colour_i[offset2 + i], colour_i[offset3 + i], n_channels == 4 ? colour_i[offset4 + i] : 0);
    } else {
      fill_rgb<From>(&rgb, colour_d[offset1 + i], colour_d[offset2 + i], colour_d[offset3 + i], n_channels == 4 ? colour_d[offset4 + i] : 0.0);
    }
    if (!rgb.valid) {
      SET_STRING_ELT(codes, i, R_NaString);
      continue;
    }
    num = double2int(rgb.r);
    num = cap0255(num) * 2;
    buf[1] = hex8[num];
    buf[2] = hex8[num + 1];
    
    num = double2int(rgb.g);
    num = cap0255(num) * 2;
    buf[3] = hex8[num];
    buf[4] = hex8[num + 1];
    
    num = double2int(rgb.b);
    num = cap0255(num) * 2;
    buf[5] = hex8[num];
    buf[6] = hex8[num + 1];
    
    if (has_alpha) {
      if (one_alpha) {
        buf[7] = alpha1;
        buf[8] = alpha2;
      } else {
        if (alpha_is_int) {
          num = alpha_i[i];
        } else {
          num = double2int(alpha_d[i]);
        }
        num = cap0255(num) * 2;
        if (num == 510) { // opaque
          buf[7] = '\0';
        } else {
          buf[7] = hex8[num];
          buf[8] = hex8[num + 1];
        }
      }
    }
    
    
    SET_STRING_ELT(codes, i, Rf_mkChar(buf));
  }
  
  copy_names(colour, codes);
  UNPROTECT(1);
  return codes;
}

template<>
SEXP encode_impl<ColorSpace::Rgb>(SEXP colour, SEXP alpha, SEXP white) {
  if (Rf_ncols(colour) < 3) {
    Rf_error("Colour in RGB format must contain at least 3 columns");
  }
  int n = Rf_nrows(colour);
  SEXP codes = PROTECT(Rf_allocVector(STRSXP, n));
  bool has_alpha = !Rf_isNull(alpha);
  char alpha1 = '\0';
  char alpha2 = '\0';
  bool alpha_is_int = false;
  bool one_alpha = false;
  char* buf = NULL;
  int* alpha_i = NULL;
  double* alpha_d = NULL;
  if (has_alpha) {
    buf = buffera;
    alpha_is_int = Rf_isInteger(alpha);
    one_alpha = Rf_length(alpha) == 1;
    int first_alpha;
    if (alpha_is_int) {
      alpha_i = INTEGER(alpha);
      first_alpha = alpha_i[0];
      first_alpha = first_alpha == R_NaInt ? 255 : first_alpha;
    } else {
      alpha_d = REAL(alpha);
      if (!R_finite(alpha_d[0])) {
        first_alpha = 255;
      } else {
        first_alpha = double2int(alpha_d[0]);
      }
    }
    first_alpha = cap0255(first_alpha) * 2;
    alpha1 = hex8[first_alpha];
    alpha2 = hex8[first_alpha + 1];
  } else {
    buf = buffer;
  }
  int offset1 = 0;
  int offset2 = offset1 + n;
  int offset3 = offset2 + n;
  
  int* colour_i = NULL;
  double* colour_d = NULL;
  bool colour_is_int = Rf_isInteger(colour);
  int num;
  if (colour_is_int) {
    int r, g, b;
    colour_i = INTEGER(colour);
    for (int i = 0; i < n; ++i) {
      r = colour_i[offset1 + i];
      g = colour_i[offset2 + i];
      b = colour_i[offset3 + i];
      if (r == R_NaInt || g == R_NaInt || b == R_NaInt) {
        SET_STRING_ELT(codes, i, R_NaString);
        continue;
      }
      num = cap0255(r) * 2;
      buf[1] = hex8[num];
      buf[2] = hex8[num + 1];
      
      num = cap0255(g) * 2;
      buf[3] = hex8[num];
      buf[4] = hex8[num + 1];
      
      num = cap0255(b) * 2;
      buf[5] = hex8[num];
      buf[6] = hex8[num + 1];
      
      if (has_alpha) {
        if (one_alpha) {
          buf[7] = alpha1;
          buf[8] = alpha2;
        } else {
          if (alpha_is_int) {
            num = alpha_i[i];
          } else {
            num = double2int(alpha_d[i]);
          }
          num = cap0255(num) * 2;
          if (num == 510) { // opaque
            buf[7] = '\0';
          } else {
            buf[7] = hex8[num];
            buf[8] = hex8[num + 1];
          }
        }
      }
      
      SET_STRING_ELT(codes, i, Rf_mkChar(buf));
    }
  } else {
    double r, g, b;
    colour_d = REAL(colour);
    for (int i = 0; i < n; ++i) {
      r = colour_d[offset1 + i];
      g = colour_d[offset2 + i];
      b = colour_d[offset3 + i];
      if (!(R_finite(r) && R_finite(g) && R_finite(b))) {
        SET_STRING_ELT(codes, i, R_NaString);
        continue;
      }
      num = cap0255(double2int(r)) * 2;
      buf[1] = hex8[num];
      buf[2] = hex8[num + 1];
      
      num = cap0255(double2int(g)) * 2;
      buf[3] = hex8[num];
      buf[4] = hex8[num + 1];
      
      num = cap0255(double2int(b)) * 2;
      buf[5] = hex8[num];
      buf[6] = hex8[num + 1];
      
      if (has_alpha) {
        if (one_alpha) {
          buf[7] = alpha1;
          buf[8] = alpha2;
        } else {
          if (alpha_is_int) {
            num = alpha_i[i];
          } else {
            num = double2int(alpha_d[i]);
          }
          num = cap0255(num) * 2;
          if (num == 510) { // opaque
            buf[7] = '\0';
          } else {
            buf[7] = hex8[num];
            buf[8] = hex8[num + 1];
          }
        }
      }
      
      SET_STRING_ELT(codes, i, Rf_mkChar(buf));
    }
  }
  
  copy_names(colour, codes);
  UNPROTECT(1);
  return codes;
}

SEXP encode_c(SEXP colour, SEXP alpha, SEXP from, SEXP white) {
  switch (INTEGER(from)[0]) {
    case CMY: return encode_impl<ColorSpace::Cmy>(colour, alpha, white);
    case CMYK: return encode_impl<ColorSpace::Cmyk>(colour, alpha, white);
    case HSL: return encode_impl<ColorSpace::Hsl>(colour, alpha, white);
    case HSB: return encode_impl<ColorSpace::Hsb>(colour, alpha, white);
    case HSV: return encode_impl<ColorSpace::Hsv>(colour, alpha, white);
    case LAB: return encode_impl<ColorSpace::Lab>(colour, alpha, white);
    case HUNTERLAB: return encode_impl<ColorSpace::HunterLab>(colour, alpha, white);
    case LCH: return encode_impl<ColorSpace::Lch>(colour, alpha, white);
    case LUV: return encode_impl<ColorSpace::Luv>(colour, alpha, white);
    case RGB: return encode_impl<ColorSpace::Rgb>(colour, alpha, white);
    case XYZ: return encode_impl<ColorSpace::Xyz>(colour, alpha, white);
    case YXY: return encode_impl<ColorSpace::Yxy>(colour, alpha, white);
    case HCL: return encode_impl<ColorSpace::Hcl>(colour, alpha, white);
  }
  
  // never happens
  return R_NilValue;
}

SEXP load_colour_names_c(SEXP name, SEXP value) {
  ColourMap& named_colours = get_named_colours();
  int n = Rf_length(name);
  if (n != Rf_ncols(value)) {
    Rf_error("name and value must have the same length");
  }
  int* values = INTEGER(value);
  int it = 0;
  for (int i = 0; i < n; ++i) {
    std::string colour_name(Rf_translateCharUTF8(STRING_ELT(name, i)));
    rgb_colour col;
    col.r = values[it++];
    col.g = values[it++];
    col.b = values[it++];
    named_colours[colour_name] = col;
  }
  return R_NilValue;
}

template <typename To>
SEXP decode_impl(SEXP codes, SEXP alpha, SEXP white) {
  bool get_alpha = LOGICAL(alpha)[0];
  int n_channels = dimension<To>();
  int n_cols = get_alpha ? n_channels + 1 : n_channels;
  int n = Rf_length(codes);
  ColourMap& named_colours = get_named_colours();
  SEXP colours = PROTECT(Rf_allocMatrix(REALSXP, n, n_cols));
  double* colours_p = REAL(colours);
  int offset1 = 0;
  int offset2 = offset1 + n;
  int offset3 = offset2 + n;
  int offset4 = offset3 + n;
  int offset5 = offset4 + n;
  
  ColorSpace::Rgb rgb;
  ColorSpace::XyzConverter::SetWhiteReference(REAL(white)[0], REAL(white)[1], REAL(white)[2]);
  To to;
  
  int nchar;
  double a;
  bool has_alpha;
  for (int i = 0; i < n; ++i) {
    SEXP code = STRING_ELT(codes, i);
    if (code == R_NaString) {
      colours_p[offset1 + i] = R_NaReal;
      colours_p[offset2 + i] = R_NaReal;
      colours_p[offset3 + i] = R_NaReal;
      if (n_cols > 3) colours_p[offset4 + i] = R_NaReal;
      if (n_cols > 4) colours_p[offset5 + i] = R_NaReal;
      continue;
    }
    const char* col = Rf_translateCharUTF8(code);
    if (col[0] == '#') {
      nchar = strlen(col);
      has_alpha = nchar == 9;
      if (!has_alpha && nchar != 7) {
        Rf_error("Malformed colour string `%s`. Must contain either 6 or 8 hex values", col);
      }
      rgb.r = hex2int(col[1]) * 16 + hex2int(col[2]);
      rgb.g = hex2int(col[3]) * 16 + hex2int(col[4]);
      rgb.b = hex2int(col[5]) * 16 + hex2int(col[6]);
      if (has_alpha) {
        a = hex2int(col[7]) * 16 + hex2int(col[8]);
        a /= 255;
      } else {
        a = 1.0;
      }
    } else {
      ColourMap::iterator it = named_colours.find(std::string(col));
      if (it == named_colours.end()) {
        colours_p[offset1 + i] = R_NaReal;
        colours_p[offset2 + i] = R_NaReal;
        colours_p[offset3 + i] = R_NaReal;
        if (n_cols > 3) colours_p[offset4 + i] = R_NaReal;
        if (n_cols > 4) colours_p[offset5 + i] = R_NaReal;
        continue;
      } else {
        rgb.r = it->second.r;
        rgb.g = it->second.g;
        rgb.b = it->second.b;
        a = 1.0;
      }
    }
    ColorSpace::IConverter<To>::ToColorSpace(&rgb, &to);
    grab<To>(to, colours_p + offset1 + i, colours_p + offset2 + i, colours_p + offset3 + i, n_channels == 4 ? colours_p + offset4 + i : colours_p);
    if (get_alpha) {
      colours_p[n_cols == 4 ? offset4 + i : offset5 + i] = a;
    }
  }
  
  copy_names(codes, colours);
  UNPROTECT(1);
  return colours;
}

template <>
SEXP decode_impl<ColorSpace::Rgb>(SEXP codes, SEXP alpha, SEXP white) {
  bool get_alpha = LOGICAL(alpha)[0];
  int n = Rf_length(codes);
  ColourMap& named_colours = get_named_colours();
  SEXP colours;
  double* colours_d = NULL;
  int* colours_i = NULL;
  if (get_alpha) {
    colours = PROTECT(Rf_allocMatrix(REALSXP, n, 4));
    colours_d = REAL(colours);
  } else {
    colours = PROTECT(Rf_allocMatrix(INTSXP, n, 3));
    colours_i = INTEGER(colours);
  }
  int offset1 = 0;
  int offset2 = offset1 + n;
  int offset3 = offset2 + n;
  int offset4 = offset3 + n;
  
  int r, g, b, nchar;
  double a;
  bool has_alpha;
  for (int i = 0; i < n; ++i) {
    SEXP code = STRING_ELT(codes, i);
    if (code == R_NaString) {
      if (get_alpha) {
        colours_d[offset1 + i] = R_NaReal;
        colours_d[offset2 + i] = R_NaReal;
        colours_d[offset3 + i] = R_NaReal;
        colours_d[offset4 + i] = R_NaReal;
      } else {
        colours_i[offset1 + i] = R_NaInt;
        colours_i[offset2 + i] = R_NaInt;
        colours_i[offset3 + i] = R_NaInt;
      }
      continue;
    }
    const char* col = CHAR(code);
    if (col[0] == '#') {
      nchar = strlen(col);
      has_alpha = nchar == 9;
      if (!has_alpha && nchar != 7) {
        Rf_error("Malformed colour string `%s`. Must contain either 6 or 8 hex values", col);
      }
      r = hex2int(col[1]) * 16 + hex2int(col[2]);
      g = hex2int(col[3]) * 16 + hex2int(col[4]);
      b = hex2int(col[5]) * 16 + hex2int(col[6]);
      if (has_alpha) {
        a = hex2int(col[7]) * 16 + hex2int(col[8]);
        a /= 255;
      } else {
        a = 1.0;
      }
    } else {
      ColourMap::iterator it = named_colours.find(std::string(Rf_translateCharUTF8(STRING_ELT(codes, i))));
      if (it == named_colours.end()) {
        if (get_alpha) {
          colours_d[offset1 + i] = R_NaReal;
          colours_d[offset2 + i] = R_NaReal;
          colours_d[offset3 + i] = R_NaReal;
          colours_d[offset4 + i] = R_NaReal;
        } else {
          colours_i[offset1 + i] = R_NaInt;
          colours_i[offset2 + i] = R_NaInt;
          colours_i[offset3 + i] = R_NaInt;
        }
        continue;
      } else {
        r = it->second.r;
        g = it->second.g;
        b = it->second.b;
        a = 1.0;
      }
    }
    if (get_alpha) {
      colours_d[offset1 + i] = (double) r;
      colours_d[offset2 + i] = (double) g;
      colours_d[offset3 + i] = (double) b;
      colours_d[offset4 + i] = a;
    } else {
      colours_i[offset1 + i] = r;
      colours_i[offset2 + i] = g;
      colours_i[offset3 + i] = b;
    }
  }
  
  copy_names(codes, colours);
  UNPROTECT(1);
  return colours;
}

SEXP decode_c(SEXP codes, SEXP alpha, SEXP to, SEXP white) {
  switch (INTEGER(to)[0]) {
    case CMY: return decode_impl<ColorSpace::Cmy>(codes, alpha, white);
    case CMYK: return decode_impl<ColorSpace::Cmyk>(codes, alpha, white);
    case HSL: return decode_impl<ColorSpace::Hsl>(codes, alpha, white);
    case HSB: return decode_impl<ColorSpace::Hsb>(codes, alpha, white);
    case HSV: return decode_impl<ColorSpace::Hsv>(codes, alpha, white);
    case LAB: return decode_impl<ColorSpace::Lab>(codes, alpha, white);
    case HUNTERLAB: return decode_impl<ColorSpace::HunterLab>(codes, alpha, white);
    case LCH: return decode_impl<ColorSpace::Lch>(codes, alpha, white);
    case LUV: return decode_impl<ColorSpace::Luv>(codes, alpha, white);
    case RGB: return decode_impl<ColorSpace::Rgb>(codes, alpha, white);
    case XYZ: return decode_impl<ColorSpace::Xyz>(codes, alpha, white);
    case YXY: return decode_impl<ColorSpace::Yxy>(codes, alpha, white);
    case HCL: return decode_impl<ColorSpace::Hcl>(codes, alpha, white);
  }
  
  // never happens
  return R_NilValue;
}

template <typename Space>
SEXP encode_channel_impl(SEXP codes, SEXP channel, SEXP value, SEXP op, SEXP white) {
  int chan = INTEGER(channel)[0];
  int operation = INTEGER(op)[0];
  int n = Rf_length(codes);
  
  bool one_value = Rf_length(value) == 1;
  int first_value_i = 0;
  double first_value_d = 0.0;
  int* value_i = NULL;
  double* value_d = NULL;
  bool value_is_int = Rf_isInteger(value);
  if (value_is_int) {
    value_i = INTEGER(value);
    first_value_i = value_i[0];
  } else {
    value_d = REAL(value);
    first_value_d = value_d[0];
  }
  
  SEXP ret = PROTECT(Rf_allocVector(STRSXP, n));
  ColorSpace::Rgb rgb;
  ColorSpace::XyzConverter::SetWhiteReference(REAL(white)[0], REAL(white)[1], REAL(white)[2]);
  Space colour;
  int num, nchar;
  ColourMap& named_colours = get_named_colours();
  
  for (int i = 0; i < n; ++i) {
    SEXP code = STRING_ELT(codes, i);
    if (code == R_NaString ||
        (value_is_int && (one_value ? first_value_i : value_i[i]) == R_NaInt) ||
        (!value_is_int && !R_finite(one_value ? first_value_d : value_d[i]))) {
      SET_STRING_ELT(ret, i, R_NaString);
      continue;
    }
    const char* col = CHAR(code);
    if (col[0] == '#') {
      nchar = strlen(col);
      if (nchar != 9 && nchar != 7) {
        Rf_error("Malformed colour string `%s`. Must contain either 6 or 8 hex values", col);
      }
      rgb.r = hex2int(col[1]) * 16 + hex2int(col[2]);
      rgb.g = hex2int(col[3]) * 16 + hex2int(col[4]);
      rgb.b = hex2int(col[5]) * 16 + hex2int(col[6]);
      strcpy(buffera, col);
    } else {
      ColourMap::iterator it = named_colours.find(std::string(col));
      if (it == named_colours.end()) {
        SET_STRING_ELT(ret, i, R_NaString);
        continue;
      }
      rgb.r = it->second.r;
      rgb.g = it->second.g;
      rgb.b = it->second.b;
      strcpy(buffera, buffer);
    }
    ColorSpace::IConverter<Space>::ToColorSpace(&rgb, &colour);
    if (value_is_int) {
      modify_channel<Space>(colour, one_value ? first_value_i : value_i[i], chan, operation);
    } else {
      modify_channel<Space>(colour, one_value ? first_value_d : value_d[i], chan, operation);
    }
    colour.Cap();
    colour.ToRgb(&rgb);
    
    if (!(R_finite(rgb.r) && R_finite(rgb.g) && R_finite(rgb.b))) {
      SET_STRING_ELT(ret, i, R_NaString);
      continue;
    }
    num = cap0255(double2int(rgb.r)) * 2;
    buffera[1] = hex8[num];
    buffera[2] = hex8[num + 1];
    
    num = cap0255(double2int(rgb.g)) * 2;
    buffera[3] = hex8[num];
    buffera[4] = hex8[num + 1];
    
    num = cap0255(double2int(rgb.b)) * 2;
    buffera[5] = hex8[num];
    buffera[6] = hex8[num + 1];
    SET_STRING_ELT(ret, i, Rf_mkChar(buffera));
  }
  
  copy_names(codes, ret);
  UNPROTECT(1);
  return ret;
}

template <>
SEXP encode_channel_impl<ColorSpace::Rgb>(SEXP codes, SEXP channel, SEXP value, SEXP op, SEXP white) {
  int chan = INTEGER(channel)[0];
  int operation = INTEGER(op)[0];
  int n = Rf_length(codes);
  
  bool one_value = Rf_length(value) == 1;
  int first_value_i = 0;
  double first_value_d = 0.0;
  int* value_i = NULL;
  double* value_d = NULL;
  bool value_is_int = Rf_isInteger(value);
  if (value_is_int) {
    value_i = INTEGER(value);
    first_value_i = value_i[0];
  } else {
    value_d = REAL(value);
    first_value_d = value_d[0];
  }
  
  SEXP ret = PROTECT(Rf_allocVector(STRSXP, n));
  int num, nchar;
  double new_val;
  ColourMap& named_colours = get_named_colours();
  
  for (int i = 0; i < n; ++i) {
    SEXP code = STRING_ELT(codes, i);
    if (code == R_NaString ||
        (value_is_int && (one_value ? first_value_i : value_i[i]) == R_NaInt) ||
        (!value_is_int && !R_finite(one_value ? first_value_d : value_d[i]))) {
      SET_STRING_ELT(ret, i, R_NaString);
      continue;
    }
    const char* col = CHAR(code);
    if (col[0] == '#') {
      nchar = strlen(col);
      if (nchar != 9 && nchar != 7) {
        Rf_error("Malformed colour string `%s`. Must contain either 6 or 8 hex values", col);
      }
      strcpy(buffera, col);
    } else {
      ColourMap::iterator it = named_colours.find(std::string(col));
      if (it == named_colours.end()) {
        SET_STRING_ELT(ret, i, R_NaString);
        continue;
      }
      num = cap0255(it->second.r) * 2;
      buffera[1] = hex8[num];
      buffera[2] = hex8[num + 1];
      
      num = cap0255(it->second.g) * 2;
      buffera[3] = hex8[num];
      buffera[4] = hex8[num + 1];
      
      num = cap0255(it->second.b) * 2;
      buffera[5] = hex8[num];
      buffera[6] = hex8[num + 1];
      buffera[7] = '\0';
    }
    switch (chan) {
    case 1: 
      new_val = mod_val(hex2int(buffera[1]) * 16 + hex2int(buffera[2]), value_is_int ? (one_value ? first_value_i : value_i[i]) : (one_value ? first_value_d : value_d[i]), operation); // Sometimes I really hate myself
      num = cap0255(new_val) * 2;
      buffera[1] = hex8[num];
      buffera[2] = hex8[num + 1];
      break;
    case 2: 
      new_val = mod_val(hex2int(buffera[3]) * 16 + hex2int(buffera[4]), value_is_int ? (one_value ? first_value_i : value_i[i]) : (one_value ? first_value_d : value_d[i]), operation);
      num = cap0255(new_val) * 2;
      buffera[3] = hex8[num];
      buffera[4] = hex8[num + 1];
      break;
    case 3:
      new_val = mod_val(hex2int(buffera[5]) * 16 + hex2int(buffera[6]), value_is_int ? (one_value ? first_value_i : value_i[i]) : (one_value ? first_value_d : value_d[i]), operation);
      num = cap0255(new_val) * 2;
      buffera[5] = hex8[num];
      buffera[6] = hex8[num + 1];
      break;
    }
    
    SET_STRING_ELT(ret, i, Rf_mkChar(buffera));
  }
  
  copy_names(codes, ret);
  UNPROTECT(1);
  return ret;
}

SEXP encode_alpha_impl(SEXP codes, SEXP value, SEXP op) {
  int operation = INTEGER(op)[0];
  int n = Rf_length(codes);
  
  bool one_value = Rf_length(value) == 1;
  int first_value_i = 0;
  double first_value_d = 0.0;
  int* value_i = NULL;
  double* value_d = NULL;
  bool value_is_int = Rf_isInteger(value);
  if (value_is_int) {
    value_i = INTEGER(value);
    first_value_i = value_i[0];
  } else {
    value_d = REAL(value);
    first_value_d = value_d[0];
  }
  
  SEXP ret = PROTECT(Rf_allocVector(STRSXP, n));
  int alpha, num, nchar;
  ColourMap& named_colours = get_named_colours();
  
  for (int i = 0; i < n; ++i) {
    SEXP code = STRING_ELT(codes, i);
    if (code == R_NaString) {
      SET_STRING_ELT(ret, i, R_NaString);
      continue;
    }
    
    const char* col = CHAR(code);
    if (col[0] == '#') {
      nchar = strlen(col);
      if (nchar != 9 && nchar != 7) {
        Rf_error("Malformed colour string `%s`. Must contain either 6 or 8 hex values", col);
      }
      strcpy(buffera, col);
      if (strlen(buffera) == 7) {
        alpha = 255;
      } else {
        alpha = hex2int(buffera[7]) * 16 + hex2int(buffera[8]);
      }
    } else {
      ColourMap::iterator it = named_colours.find(std::string(col));
      if (it == named_colours.end()) {
        SET_STRING_ELT(ret, i, R_NaString);
        continue;
      }
      num = cap0255(it->second.r) * 2;
      buffera[1] = hex8[num];
      buffera[2] = hex8[num + 1];
      
      num = cap0255(it->second.g) * 2;
      buffera[3] = hex8[num];
      buffera[4] = hex8[num + 1];
      
      num = cap0255(it->second.b) * 2;
      buffera[5] = hex8[num];
      buffera[6] = hex8[num + 1];
      alpha = 255;
    }
    
    if (value_is_int) {
      alpha = double2int(mod_val(alpha / 255.0, one_value ? first_value_i : value_i[i], operation) * 255.0);
    } else {
      alpha = double2int(mod_val(alpha / 255.0, one_value ? first_value_d : value_d[i], operation) * 255.0);
    }
    alpha = cap0255(alpha);
    if (alpha == 255) {
      buffera[7] = '\0';
    } else {
      num = alpha * 2;
      buffera[7] = hex8[num];
      buffera[8] = hex8[num + 1];
    }
    SET_STRING_ELT(ret, i, Rf_mkChar(buffera));
  }
  
  copy_names(codes, ret);
  UNPROTECT(1);
  return ret;
}

SEXP encode_channel_c(SEXP codes, SEXP channel, SEXP value, SEXP space, SEXP op, SEXP white) {
  if (INTEGER(channel)[0] == 0) {
    return encode_alpha_impl(codes, value, op);
  }
  switch (INTEGER(space)[0]) {
    case CMY: return encode_channel_impl<ColorSpace::Cmy>(codes, channel, value, op, white);
    case CMYK: return encode_channel_impl<ColorSpace::Cmyk>(codes, channel, value, op, white);
    case HSL: return encode_channel_impl<ColorSpace::Hsl>(codes, channel, value, op, white);
    case HSB: return encode_channel_impl<ColorSpace::Hsb>(codes, channel, value, op, white);
    case HSV: return encode_channel_impl<ColorSpace::Hsv>(codes, channel, value, op, white);
    case LAB: return encode_channel_impl<ColorSpace::Lab>(codes, channel, value, op, white);
    case HUNTERLAB: return encode_channel_impl<ColorSpace::HunterLab>(codes, channel, value, op, white);
    case LCH: return encode_channel_impl<ColorSpace::Lch>(codes, channel, value, op, white);
    case LUV: return encode_channel_impl<ColorSpace::Luv>(codes, channel, value, op, white);
    case RGB: return encode_channel_impl<ColorSpace::Rgb>(codes, channel, value, op, white);
    case XYZ: return encode_channel_impl<ColorSpace::Xyz>(codes, channel, value, op, white);
    case YXY: return encode_channel_impl<ColorSpace::Yxy>(codes, channel, value, op, white);
    case HCL: return encode_channel_impl<ColorSpace::Hcl>(codes, channel, value, op, white);
  }
  
  // never happens
  return R_NilValue;
}

template <typename Space>
SEXP decode_channel_impl(SEXP codes, SEXP channel, SEXP white) {
  int chan = INTEGER(channel)[0];
  int n = Rf_length(codes);
  
  SEXP ret = PROTECT(Rf_allocVector(REALSXP, n));
  double* ret_p = REAL(ret);
  ColorSpace::Rgb rgb;
  ColorSpace::XyzConverter::SetWhiteReference(REAL(white)[0], REAL(white)[1], REAL(white)[2]);
  Space colour;
  ColourMap& named_colours = get_named_colours();
  int nchar;
  
  for (int i = 0; i < n; ++i) {
    SEXP code = STRING_ELT(codes, i);
    if (code == R_NaString) {
      ret_p[i] = R_NaReal;
      continue;
    }
    const char* col = CHAR(code);
    if (col[0] == '#') {
      nchar = strlen(col);
      if (nchar != 9 && nchar != 7) {
        Rf_error("Malformed colour string `%s`. Must contain either 6 or 8 hex values", col);
      }
      rgb.r = hex2int(col[1]) * 16 + hex2int(col[2]);
      rgb.g = hex2int(col[3]) * 16 + hex2int(col[4]);
      rgb.b = hex2int(col[5]) * 16 + hex2int(col[6]);
    } else {
      ColourMap::iterator it = named_colours.find(std::string(col));
      if (it == named_colours.end()) {
        ret_p[i] = R_NaReal;
        continue;
      }
      rgb.r = it->second.r;
      rgb.g = it->second.g;
      rgb.b = it->second.b;
    }
    ColorSpace::IConverter<Space>::ToColorSpace(&rgb, &colour);
    colour.Cap();
    
    ret_p[i] = grab_channel<Space>(colour, chan);
  }
  
  copy_names(codes, ret);
  UNPROTECT(1);
  return ret;
}

template <>
SEXP decode_channel_impl<ColorSpace::Rgb>(SEXP codes, SEXP channel, SEXP white) {
  int chan = INTEGER(channel)[0];
  int n = Rf_length(codes);
  
  SEXP ret = PROTECT(Rf_allocVector(INTSXP, n));
  int* ret_p = INTEGER(ret);
  ColourMap& named_colours = get_named_colours();
  int val = 0;
  int nchar = 0;
  
  for (int i = 0; i < n; ++i) {
    SEXP code = STRING_ELT(codes, i);
    if (code == R_NaString) {
      ret_p[i] = R_NaInt;
      continue;
    }
    const char* col = CHAR(code);
    if (col[0] == '#') {
      nchar = strlen(col);
      if (nchar != 9 && nchar != 7) {
        Rf_error("Malformed colour string `%s`. Must contain either 6 or 8 hex values", col);
      }
      switch (chan) {
      case 1: 
        val = hex2int(col[1]) * 16 + hex2int(col[2]);
        break;
      case 2: 
        val = hex2int(col[3]) * 16 + hex2int(col[4]);
        break;
      case 3: 
        val = hex2int(col[5]) * 16 + hex2int(col[6]);
        break;
      }
    } else {
      ColourMap::iterator it = named_colours.find(std::string(col));
      if (it == named_colours.end()) {
        ret_p[i] = R_NaInt;
        continue;
      }
      switch (chan) {
      case 1: 
        val = it->second.r;
        break;
      case 2: 
        val = it->second.g;
        break;
      case 3: 
        val = it->second.b;
        break;
      }
    }
    
    ret_p[i] = val;
  }
  
  copy_names(codes, ret);
  UNPROTECT(1);
  return ret;
}

SEXP decode_alpha_impl(SEXP codes) {
  int n = Rf_length(codes);
  
  SEXP ret = PROTECT(Rf_allocVector(REALSXP, n));
  double* ret_p = REAL(ret);
  ColourMap& named_colours = get_named_colours();
  int nchar;
  bool has_alpha;
  double val;
  
  for (int i = 0; i < n; ++i) {
    SEXP code = STRING_ELT(codes, i);
    if (code == R_NaString) {
      ret_p[i] = R_NaInt;
      continue;
    }
    const char* col = CHAR(code);
    if (col[0] == '#') {
      nchar = strlen(col);
      has_alpha = nchar == 9;
      if (!has_alpha && nchar != 7) {
        Rf_error("Malformed colour string `%s`. Must contain either 6 or 8 hex values", col);
      }
      if (has_alpha) {
        val = (hex2int(col[7]) * 16 + hex2int(col[8])) / 255.0;
      } else {
        val = 1.0;
      }
    } else {
      ColourMap::iterator it = named_colours.find(std::string(col));
      if (it == named_colours.end()) {
        ret_p[i] = R_NaReal;
        continue;
      }
      val = 1.0;
    }
    
    ret_p[i] = val;
  }
  
  copy_names(codes, ret);
  UNPROTECT(1);
  return ret;
}

SEXP decode_channel_c(SEXP codes, SEXP channel, SEXP space, SEXP white) {
  if (INTEGER(channel)[0] == 0) {
    return decode_alpha_impl(codes);
  }
  switch (INTEGER(space)[0]) {
  case CMY: return decode_channel_impl<ColorSpace::Cmy>(codes, channel, white);
  case CMYK: return decode_channel_impl<ColorSpace::Cmyk>(codes, channel, white);
  case HSL: return decode_channel_impl<ColorSpace::Hsl>(codes, channel, white);
  case HSB: return decode_channel_impl<ColorSpace::Hsb>(codes, channel, white);
  case HSV: return decode_channel_impl<ColorSpace::Hsv>(codes, channel, white);
  case LAB: return decode_channel_impl<ColorSpace::Lab>(codes, channel, white);
  case HUNTERLAB: return decode_channel_impl<ColorSpace::HunterLab>(codes, channel, white);
  case LCH: return decode_channel_impl<ColorSpace::Lch>(codes, channel, white);
  case LUV: return decode_channel_impl<ColorSpace::Luv>(codes, channel, white);
  case RGB: return decode_channel_impl<ColorSpace::Rgb>(codes, channel, white);
  case XYZ: return decode_channel_impl<ColorSpace::Xyz>(codes, channel, white);
  case YXY: return decode_channel_impl<ColorSpace::Yxy>(codes, channel, white);
  case HCL: return decode_channel_impl<ColorSpace::Hcl>(codes, channel, white);
  }
  
  // never happens
  return R_NilValue;
}
