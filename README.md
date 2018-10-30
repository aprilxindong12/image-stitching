# image-stitching

## To print usage
stitch --help

## To run the program
Example 1 (one argument you must specify):

'stitch an_image_folder'

Example 2 (one compulsory argument and other optional flags):

'''
stitch an_image_folder --image_ext bmp --stitchForward 1 --work_megapix 0.6
--max_no_of_stitched_parts 10 --no_of_overlapping_frames 50 --max_no_of_mismatches 10 --min_no_of_rows 1 --min_no_of_inliers_1 8 --match_conf_1 0.3 --scaling_range_1 0.1
--min_no_of_inliers_2 8 --match_conf_2 0.2 --scaling_range_2 0.2 --roi_rows 2.0
'''

## Outputs
Stitched parts;

Result panorama, result panorama with transparent borders, result panorama mask (for mapping purposes);

outputs.xml (stitching parameters and summary of results);

matrices.xml (for mapping purposes); 

csv_outputs.csv (stitching information).

### Interpreting "csv_outputs.csv"
no_of_rows/new_no_of_rows:

	>0: number of new rows from the image or stitched part;
	
	 0: no new rows in the image or stitched part;
	 
	-1: not enough no_of_inliers in the image or stitched part;
	
	-2: image or stitched part out of the allowed range of rotation or the estimated transformation is incorrect;
	
	-3: estimated transformation of the image or stitched part is out of the allowed range of scaling.
