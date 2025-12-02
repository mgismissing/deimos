rm -rf tmp
mkdir -p tmp/raw
mkdir -p tmp/bw

OUT="$(dirname $1)/$(basename $1 .mp4).raw"

ffmpeg -i $1 -vf "fps=1" tmp/raw/%06d.png

rm -f $OUT
touch $OUT
for f in tmp/raw/*.png; do
    base=$(basename $f .png)
    magick $f \
        -resize 192x63! \
        -dither FloydSteinberg \
        -remap pattern:gray50 \
        -monochrome \
        -depth 1 \
        -negate \
        gray:tmp/bw/$base.raw
    dd if=tmp/bw/$base.raw of=$OUT bs=1 seek=$(stat -c%s $OUT) conv=notrunc
done
rm -rf tmp