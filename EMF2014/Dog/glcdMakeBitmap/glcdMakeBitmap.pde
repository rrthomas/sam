/**
 * glcdBitmap.
 *
 * Creates a bitmap definition file that can be included in an Arduino sketch
 * to display a bitmap image on a graphics LCD using the Arduino GLCD library.
 *
 * Created  6 Nov  2008  Copyright Michael Margolis 2008,2010
 * updated  14 Feb 2010
 * converted to command-line operation by Reuben Thomas 12th November 2020
 */

PImage bitmap;

static class imgData {
  static int width;
  static int height;
  static int pages;
  static int bytes;
  static int pixels;
  static String sourceFileName; // excludes path
  static String baseName;       // excludes path and extension
}

void setup() {
 if (args != null)
    for (int i = 0; i < args.length; i++)
      convert(args[i]);
}

void convert(String sourcePath) {
  bitmap = loadImage(sourcePath);   // load the image
  if( bitmap != null) {
    imgData.width = bitmap.width;
    imgData.height = bitmap.height;
    imgData.pages = (imgData.height + 7)/8; // round up so each page contains 8 pixels
    imgData.bytes = imgData.width * imgData.pages;
    imgData.pixels = imgData.width * imgData.height;
    image(bitmap,0,0);
    print("Width = ");
    println(imgData.width);
    print("Height = ");
    println(imgData.height);
    print("Pages = ");
    println(imgData.pages);
    print("Image bytes = ");
    println(imgData.bytes);
    print("Pixels = ");
    println(imgData.pixels);

    bitmap.loadPixels();
    imgData.baseName = getBaseName(sourcePath);
    imgData.sourceFileName = new File(sourcePath).getName();
    writeFile(); // writes a file using arguments defined in the imgData structure

    //println("created header file for " + sourcePath);    // prints the full path
    println("\nCreated " + imgData.baseName + ".h" + " using " + imgData.sourceFileName); // show on command line
    text("Created file: " + imgData.baseName + ".h", 20,height - 10); // display text in window
  } else {
    println("Unable to load image");
    text("Unable to load image", 20,height - 10);
  }
}

void writeFile() {
  print("Basename = ");
  println(imgData.baseName);
  String outFileName = "../" + imgData.baseName + ".h";
  print("Output file name = ");
  println(outFileName);

  PrintWriter output;
  output = createWriter(outFileName);

  output.println("/* " + imgData.baseName + " bitmap file for GLCD library */");
  output.println("/* Bitmap created from " +  imgData.sourceFileName + "      */");
  String[] monthName = { "","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"  };
  output.println("/* Date: " +  day() + " " + monthName[month()] +  " " +  year() + "      */" );
  output.println("/* Image Pixels = "  + imgData.pixels + "    */");
  output.println("/* Image Bytes  = "   + imgData.bytes  + "     */");
  output.println();

  output.println("#include <inttypes.h>");
  output.println("#include <avr/pgmspace.h>");
  output.println();
  output.println("#ifndef " +  imgData.baseName + "_H");
  output.println("#define " +  imgData.baseName + "_H");

  output.println();
  output.print("static uint8_t ");
  output.print( imgData.baseName);
  output.println("[] PROGMEM = {");

  output.print("  ");
  output.print(imgData.width);   // note width and height are bytes so 256 will be 0
  output.println(", // width");
  output.print("  ");
  output.print(imgData.height);
  output.println(", // height");
  stroke(0);
  for (int page=0; page < imgData.pages; page++) {
    output.println("\n  /* page " + page + " (lines "  + page * 8 + "-" + (page * 8 + 7) + ") */");
    output.print("  ");
    for(int x=0; x < imgData.width; x++) {
      output.print( "0x" + Integer.toHexString(getValue(x , page))) ;
      if (x == width - 1 && page == ((height + 7) / 8) - 1)
        println("\n"); // this is the last element so new line instead of comma
      else
        output.print(",");   // comma on all but last entry
      if (x % 16 == 15)
        output.print("\n  ");
    }
  }
  output.print("\n};\n");
  output.println("#endif");

  output.flush(); // Write the remaining data
  output.close(); // Finish the file
}

// return the byte representing data a the given page and x offset
int getValue( int x, int page) {
  //print("page = ");println(page);
  int val = 0;
  for (byte bit = 0; bit < 8; bit++) {
    int y = page * 8 + bit;
    int pos = y * imgData.width + x;
    if (pos < imgData.width * imgData.height) { // skip padding if at the end of real data
      int c = bitmap.pixels[pos];    // get the color
      int r = (c >> 16) & 0xFF;      // get the rgb values
      int g = (c >>  8) & 0xFF;
      int b = c         & 0xFF;
      if (r < 128 || g < 128 || b < 128) // test if any value is closer to dark than light
        val |= (1 << bit);   // set the bit if this pixel is more dark than light
    }
  }
  return val;
}

public String getBaseName(String fileName) {
  File tmpFile = new File(fileName);
  tmpFile.getName();
  int whereDot = tmpFile.getName().lastIndexOf('.');
  if (0 < whereDot && whereDot <= tmpFile.getName().length() - 2)
    return tmpFile.getName().substring(0, whereDot);
    //extension = filename.substring(whereDot + 1);
  return "";
}
