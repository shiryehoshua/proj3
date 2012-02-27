
There is no need for you to worry about how these images were created,
and making new images is not an expected part of the project.  The
descriptions of the images here include information about how they
were made, which may be of interest for those who want to experiment
with other images (though doing so is not going to get you extra
credit).  Further info about the command-line tools used is below.

*** check-rgb.png is a test pattern that may be useful for testing
different texture filtering options

*** hand-512.png and hand-x2.png are images that might be helpful for
visualizing the relationship between the way that the PNG images
appear when viewed as stand-alone images, and the way that they appear
when used as texture maps in OpenGL.  The hand-x2.png image, because
it is not a square image, may also be a good test for doing the manual
indexing into the texture-map to do per-vertex coloring.

*** arrows-512.png is intended to be useful when debugging the
wrapping problem for texture mapping round objects for which the
first texture coordinate changes suddenly from 0.0 to 1.0.  Any
errors in how the texture is repeated will be visible as kinks
in what should be smoothly c urving arrows.  arrows-rgb.png is a 
more colorful version of the same idea.

*** hemisph-*.png are images to help debug bump mapping and parallax
mapping.  The intended geometry is that of a radius-1 hemisphere
sticking out of the plane, with plane cutting through the equator of
the sphere.  hemisph-norm08.png and hemisph-norm16.png are 8-bit and
16-bit versions of the normal map, which encodes the normal in the
tangent space of the surface.  If this bump map is applied to a square
(with the view direction aligned with the square normal), then shading
using this bump map should look like exactly like a hemisphere.  The
hemisph-hght08.png and hemisph-hght16.png are 8-bit and 16-bit
quantizations of the height (from 0.0 to 1.0), to be used for parallax
mapping.  It is expected that the effect will break down some at the
boundary of the hemisphere, where the change in height is nearly
discontinuous.  hemisph-rgb.png is a colormap of the hemisphere
according to height: the color bands span equal distances along the
height axis.

*** uchic-rgb.png is the University Seal

*** uchic-norm08.png is an 8-bit normal map to be used for bump
mapping with uchic-rgb.png

*** brick.png is based on:
http://paulbourke.net/texture_colour/displayimage.cgi?brick/brick29.jpg
See http://paulbourke.net/texture_colour/
for many more possible textures to play with

*** globe-1024.png is a equirectangular map of the globe, from
wikipedia, created as follows:

  curl http://upload.wikimedia.org/wikipedia/commons/thumb/e/ea/Equirectangular-projection.jpg/1280px-Equirectangular-projection.jpg > tmp.jpg
  djpeg tmp.jpg | unu resample -s = 1024 512 -b wrap -o globe-1024.png
  rm -f tmp.jpg

*** spot-circle.png was made with:
  echo "-1 1 -1 1" | unu reshape -s 2 2 \
   | unu resample -s 256 256 -k tent -c node -o x
  unu swap -i x -a 0 1 -o y
  unu join -i x y -a 0 -incr \
   | unu project -a 0 -m l2 \
   | unu 2op - 0.92 - \
   | unu 2op x - 18 \
   | unu 1op erf \
   | unu quantize -b 8 -o gray.png
  unu join -i gray.png gray.png gray.png -a 0 -incr -o spot-circle.png 
  rm -f x y gray.png

*** cube-sample.png is based on an image from Wikipedia, made with:

  curl -O http://upload.wikimedia.org/wikipedia/commons/e/e2/Panorama_cube_map.png 
  unu crop -i Panorama_cube_map.png -min 0 0 0 -max 2 M 1535 ...
  ...
  rm -f Panorama_cube_map.png 



To make the normal map images from a height image, the general procedure is:

   unu resample -i hght.nrrd -s x1 = -k centdiff -b wrap \
    | unu 2op x - -${SCL} -o dx.nrrd
   unu resample -i hght.nrrd -s = x1 -k centdiff \
    | unu 2op x - -${SCL} -o dy.nrrd
   unu 1op exists -i dx.nrrd -o 1.nrrd
   
   unu join -i {dx,dy,1}.nrrd -a 0 -incr -o g.nrrd
   unu project -i g.nrrd -a 0 -m l2 \
    | unu axinsert -a 0 \
    | unu pad -min 0 0 0 -max 2 M M \
    | unu 2op / g.nrrd - -o n.nrrd
   
   unu quantize -b 8 -i n.nrrd -min -1 -max 1 -o norm08.png
   unu quantize -b 16 -i n.nrrd -min -1 -max 1 -o norm16.png
   
   rm -f {hght,dx,dy,1,g,n,z}.nrrd

where the height field is in hght.nrrd (or maybe hght.png), and ${SCL}
is the variable that determines the effective height, by scaling the X
and Y components of the surface normal relative to a fixed constant Z
component of 1.  The first two "unu resamples" take the derivatives
along the faster and slower axes, respectively, by central
differences.

About the command-line tools:
djpeg: is a command-line JPG decompressor from libjpeg
curl: command-line URL grabber http://curl.haxx.se/
unu: command-line tool for N-D array hacking, compiled
  as part of Teem: http://teem.sourceforge.net/build.html
