#pragma once

class MATRIX7219;

/// masks: index is the mask width i.e.
/// - index 0 -> 0b00000000
/// - index 2 -> 0b00000011
static const uint8_t masks[] = { 
  0b00000000,
  0b00000001,
  0b00000011,
  0b00000111,
  0b00001111,
  0b00011111,
  0b00111111,
  0b01111111,
  0b11111111
};

static const uint8_t display_height = 8;
static const uint8_t display_width = 8;

/// representation of a char; max size is 8x8
struct Char
{
  using data_t = std::array<uint8_t, display_height>;
  
  Char(uint8_t width_, data_t&& data_)
    : width(width_)
    , data(std::move(data_))
  {}

  // returns data for row masked according to width
  uint8_t row(uint8_t r) const
  {
    return data[r] & masks[width];  
  }

  const uint8_t width;
  const data_t data;
};

const static Char s_colon( 
  2, 
  {0b00000000,
   0b00000000,
   0b00000001,
   0b00000001,
   0b00000000,
   0b00000001,
   0b00000001,
   0b00000000});
const static Char s_fullstop( 
  1, 
  {0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000001,
   0b00000000}); 
const static Char s_one( 
  5, 
  {0b00000000,
   0b00000010,
   0b00000110,
   0b00000010,
   0b00000010,
   0b00000010,
   0b00000111,
   0b00000000}); 
const static Char s_two( 
  5, 
  {0b00000000,
   0b00000110,
   0b00001001,
   0b00000001,
   0b00000010,
   0b00000100,
   0b00001111,
   0b00000000}); 
const static Char s_three( 
  5, 
  {0b00000000,
   0b00000110,
   0b00001001,
   0b00000010,
   0b00000001,
   0b00001001,
   0b00000110,
   0b00000000}); 
const static Char s_four( 
  5, 
  {0b00000000,
   0b00000010,
   0b00000110,
   0b00001010,
   0b00001111,
   0b00000010,
   0b00000010,
   0b00000000}); 
const static Char s_five( 
  5, 
  {0b00000000,
   0b00001111,
   0b00001000,
   0b00001110,
   0b00000001,
   0b00001001,
   0b00000110,
   0b00000000}); 
const static Char s_six( 
  5, 
  {0b00000000,
   0b00000110,
   0b00001000,
   0b00001110,
   0b00001001,
   0b00001001,
   0b00000110,
   0b00000000}); 
const static Char s_seven( 
  5, 
  {0b00000000,
   0b00001111,
   0b00000001,
   0b00000010,
   0b00000010,
   0b00000100,
   0b00000100,
   0b00000000}); 
const static Char s_eight( 
  5, 
  {0b00000000,
   0b00000110,
   0b00001001,
   0b00000110,
   0b00001001,
   0b00001001,
   0b00000110,
   0b00000000}); 
const static Char s_nine( 
  5, 
  {0b00000000,
   0b00000110,
   0b00001001,
   0b00001001,
   0b00000111,
   0b00000001,
   0b00000110,
   0b00000000}); 
const static Char s_zero( 
  5, 
  {0b00000000,
   0b00000110,
   0b00001001,
   0b00001001,
   0b00001001,
   0b00001001,
   0b00000110,
   0b00000000}); 

static const Char digits[] = {
  s_zero,
  s_one,
  s_two,
  s_three,
  s_four,
  s_five,
  s_six,
  s_seven,
  s_eight,
  s_nine
};

/// store displayable string as a series of rows,
/// left most is MSB
struct Display
{
  using data_t = std::array<uint64_t, display_height>;

  enum Alignment
  {
    LEFT,
    CENTER,
    RIGHT
  };

  Display( MATRIX7219& m )
    : matrix_(m)
    , total_width_(m.getMatrixCount() * display_width)
  {
    matrix_.begin();
    matrix_.clear();  
  }
     
  /// extract data for one row in one matrix; matrix is
  /// zero indexed
  uint8_t get_matrix_row(uint8_t matrix, uint8_t row) const
  {
    uint64_t data = rows_[row];
    switch (alignment_) {
      case LEFT:
        data <<= total_width_ - column_offset_;
        break;
      case CENTER:
        data <<= (total_width_ - column_offset_)/2;
        break;
      case RIGHT:
      default:
        break;
    }
    data >>= (matrix * display_width);
    return data;
  }

  Alignment alignment() const { return alignment_; }
  void set_alignment(Alignment a) { alignment_ = a; }

  void add_char(const Char& c)
  {
    for ( uint8_t r_index=0; r_index<display_height; ++r_index )
    {
      uint64_t r = c.row(r_index);
      r <<= column_offset_;
      rows_[r_index] |= r;
    }

    column_offset_ += c.width;
  }

  void clear()
  {
    rows_ = {};
    column_offset_ = 0;
  }

  void render()
  {
    for ( uint8_t m = 0; m < matrix_.getMatrixCount(); ++m )
      for ( uint8_t r = 0; r < display_height; ++r )
      {
        matrix_.setRow(1 + r, get_matrix_row(m, r), 1 + m);
      }
  }

  MATRIX7219& matrix_;
  const uint8_t total_width_;
  data_t rows_ {};
  uint8_t column_offset_ {};
  Alignment alignment_ { CENTER };
};
