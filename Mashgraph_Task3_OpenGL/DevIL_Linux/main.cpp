#include "IL/il.h"
#include <stdlib.h> /* because of malloc() etc. */
#include <IL/devil_cpp_wrapper.hpp>

int main(int argc, const char * argv[])
{
    ILuint handle, w, h;
    
	/* First we initialize the library. */
	/*Do not forget that... */
	ilInit();
	
	/* We want all images to be loaded in a consistent manner */
	ilEnable(IL_ORIGIN_SET);
	
	/* In the next section, we load one image */
	ilGenImages(1, & handle);
	ilBindImage(handle);
	ILboolean loaded = ilLoadImage("original_file.jpg");
	
	if (loaded == IL_FALSE)
		return -1; /* error encountered during loading */
	
	/* Let's spy on it a little bit */
	w = ilGetInteger(IL_IMAGE_WIDTH); // getting image width
	h = ilGetInteger(IL_IMAGE_HEIGHT); // and height
	printf("Our image resolution: %dx%d\n", w, h);
	
	/* how much memory will we need? */
	int memory_needed = w * h * 3 * sizeof(unsigned char);
	
	/* We multiply by 3 here because we want 3 components per pixel */
	ILubyte * data = (ILubyte *)malloc(memory_needed);
	
	/* finally get the image data */
	ilCopyPixels(0, 0, 0, w, h, 1, IL_RGB, IL_UNSIGNED_BYTE, data);
	
	/* We want to do something with the image, right? */
	int i;
	for(i = 0; i < memory_needed; i++)
	    if(i % 31 == 0) /* vandalise the image */
	        data[i] = i % 255;
	
	/* And maybe we want to save that all... */
	ilSetPixels(0, 0, 0, w, h, 1, IL_RGB, IL_UNSIGNED_BYTE, data);
	
	/* and dump them to the disc... */
	ilSaveImage("our_result.png");
	
	/* Finally, clean the mess! */
	ilDeleteImages(1, & handle);
	free(data); data = NULL;
	
	return 0;
}
