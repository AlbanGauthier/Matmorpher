smooth_contours: smooth_contours_cmd.c io.c io.h smooth_contours.c smooth_contours.h
	$(CC) -O3 -o smooth_contours smooth_contours_cmd.c io.c smooth_contours.c -lm

test: smooth_contours
	./smooth_contours image.pgm -t output.txt -p output.pdf

clean:
	rm -f smooth_contours output.txt output.pdf
