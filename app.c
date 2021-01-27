
/******************************************************************
* File: dbs.c
* Authors: Yi Yang, Purdue University (built upon David Lieberman code)
* Date: Dec 2017
* Implementing: Jointly design Cyan, Magenta and Yellow screens using DBS
* It can write out matrix as "txt" or halftone image as "tif"
* Note that code can be easily changed so that
*******************************************************************/


#include "dbs.h"
#include <stdint.h>
#include "allocate.h"

Config* getConfigurations();

/* The entry point of the code.*/
int main(int argc, char **argv) {

	time_t blockStart;
	time_t blockEnd;
	
	int randomizationSeed = 0;
	double maxGrayLevel = 255.0;
	
	Config *config = getConfigurations();

	struct doubleImage *psf = generateHvsFunction(config);
	struct doubleImage *cpp = generateCpp(psf);

	struct doubleImage *inputImage = readDoubleImage(config->inputImagePath, maxGrayLevel, config->gamma);
	struct doubleImage *inputImage2 = readDoubleImage(config->inputImagePath2, maxGrayLevel, config->gamma);
	//struct doubleImage *inputImage3 = readDoubleImage(config->inputImagePath3, maxGrayLevel, config->gamma);

	struct pxm_img *halftoneCMY = getInitialHalftone(config->initialHalftonePath, inputImage, maxGrayLevel, randomizationSeed);
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------


	struct pxm_img *halftoneCM = (struct pxm_img*) multialloc(sizeof(struct pxm_img), 1, 1);
	halftoneCM->height = halftoneCMY->height;
	halftoneCM->width = halftoneCMY->width;
	halftoneCM->pxm_type = 'g';
	halftoneCM->mono = (uint8_t **) get_img(halftoneCM->width, halftoneCM->height, sizeof(uint8_t));

	struct pxm_img *halftoneC = (struct pxm_img*) multialloc(sizeof(struct pxm_img), 1, 1);
	halftoneC->height = halftoneCMY->height;
	halftoneC->width = halftoneCMY->width;
	halftoneC->pxm_type = 'g';
	halftoneC->mono = (uint8_t **) get_img(halftoneC->width, halftoneC->height, sizeof(uint8_t));

	struct pxm_img *halftoneM = (struct pxm_img*) multialloc(sizeof(struct pxm_img), 1, 1);
	halftoneM->height = halftoneCMY->height;
	halftoneM->width = halftoneCMY->width;
	halftoneM->pxm_type = 'g';
	halftoneM->mono = (uint8_t **) get_img(halftoneM->width, halftoneM->height, sizeof(uint8_t));

	struct pxm_img *halftoneY = (struct pxm_img*) multialloc(sizeof(struct pxm_img), 1, 1);
	halftoneY->height = halftoneCMY->height;
	halftoneY->width = halftoneCMY->width;
	halftoneY->pxm_type = 'g';
	halftoneY->mono = (uint8_t **) get_img(halftoneY->width, halftoneY->height, sizeof(uint8_t));


	//-------------------------------------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------


	separateCM (halftoneCMY, halftoneCM, halftoneY, 0.66666, 0.33333);
	separateCM (halftoneCM, halftoneC, halftoneM, 0.5, 0.5);


	double test71 =  countNum(halftoneC);
	double test72 =  countNum(halftoneM);
	double test73 =  countNum(halftoneY);
	printf("The halftoneC, M, Y is %f, %f, %f\n ", test71, test72,test73);

	//-------------------------------------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------

	struct doubleImage *cpeCMY = calculateCpe(inputImage, halftoneCMY, cpp);
	struct doubleImage *cpeC = calculateCpe(inputImage2, halftoneC, cpp);
	struct doubleImage *cpeM = calculateCpe(inputImage2, halftoneM, cpp);
	struct doubleImage *cpeY = calculateCpe(inputImage2, halftoneY, cpp);


	for (int i = 0; i<10; i++){

		printf("Iteration %d : Jointly optimize C and Y patterns  \n", i+1);
		performCompleteDBSForScreenDesign(config,inputImage2,halftoneCMY,cpeCMY,halftoneC,cpeC,halftoneY,cpeY,
				halftoneCMY,halftoneY, halftoneC, cpp, 1);

		printf("Iteration %d :Jointly optimize M and Y patterns \n",i+1);
		performCompleteDBSForScreenDesign(config,inputImage2,halftoneCMY,cpeCMY,halftoneM, cpeM, halftoneY,cpeY,
				halftoneCMY,halftoneY, halftoneM, cpp, 1);

		printf("Iteration %d :Jointly optimize C and M patterns\n",i+1);
		performCompleteDBSForScreenDesign(config,inputImage2,halftoneCMY,cpeCMY,halftoneC,cpeC,halftoneM,cpeM,
				halftoneCMY,halftoneM, halftoneC, cpp, 1);
	}

	struct pxm_img *htY =  samepattern(halftoneY);
	struct pxm_img *htC =  samepattern(halftoneC);
	struct pxm_img *htM =  samepattern(halftoneM);

	struct pxm_img *ht2Y =  samepattern(halftoneY);



	struct doubleImage *matrixC = AllocateMatrix(config->MatrixSize);
	struct doubleImage *matrixM = AllocateMatrix(config->MatrixSize);
	struct doubleImage *matrixY = AllocateMatrix(config->MatrixSize);

	//-------------------------------------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------

	for (unsigned int seqId = 1; seqId <=85; seqId++){

		fprintf(stdout,"\n***********************************************************************************************************");
		fprintf(stdout, "\n \t\t\t  MATRIX size %u Processing order: 85->0 (Level %u)", config->MatrixSize, (85 - seqId +1));
		fprintf(stdout,"\n*********************************************************************************************************\n");
		fflush(stdout);

		//current level
		double currentlevel = (double) (85 - seqId +1);
		int level = (int)currentlevel;
		//double level = (double) (generationSeq - seqId);

		struct pxm_img* beforeC =  samepattern(halftoneC);
		struct pxm_img* beforeM =  samepattern(halftoneM);
		struct pxm_img* beforeY =  samepattern(halftoneY);

		double differ = 85 - currentlevel;

		halftoneC = removeDots(halftoneC, differ, 4, config->MatrixSize, config->MaxLevel,  1);
		halftoneM = removeDots(halftoneM, differ, 4, config->MatrixSize, config->MaxLevel,  1);
		halftoneY = removeDots(halftoneY, differ, 4, config->MatrixSize, config->MaxLevel,  1);

		printf("TEST \n");
		double test325 =  countNum(halftoneC);
		double test326 =  countNum(halftoneM);
		double test327 =  countNum(halftoneY);
		printf("C, M ,Y after remove  %f\n ", test325, test326, test327);

		struct doubleImage *inputImageC1 = generateCTImage(halftoneC);

		cpeC = calculateCpe(inputImageC1, halftoneC, cpp);
		cpeM = calculateCpe(inputImageC1, halftoneM, cpp);
		cpeY = calculateCpe(inputImageC1, halftoneY, cpp);

		// Design uniform pattern respectively

		performCompleteDBSForScreenDesign(config, inputImageC1,halftoneC,cpeC,halftoneC,cpeC,
				halftoneC, cpeC, beforeC,beforeC, beforeC, cpp, 2);

		performCompleteDBSForScreenDesign(config, inputImageC1,halftoneM,cpeM,halftoneM,cpeM,
				halftoneM, cpeM, beforeM,beforeM, beforeM, cpp, 2);

		performCompleteDBSForScreenDesign(config, inputImageC1,halftoneY,cpeY,halftoneY,cpeY,
				halftoneY, cpeY, beforeY,beforeY, beforeY, cpp, 2);




		struct pxm_img *differC = (struct pxm_img*) multialloc(sizeof(struct pxm_img), 1, 1);
		differC->height = halftoneC->height;
		differC->width = halftoneC->width;
		differC->pxm_type = 'g';
		differC->mono = (uint8_t **) get_img(differC->width, differC->height, sizeof(uint8_t));

		struct pxm_img *differM = (struct pxm_img*) multialloc(sizeof(struct pxm_img), 1, 1);
		differM->height = halftoneC->height;
		differM->width = halftoneC->width;
		differM->pxm_type = 'g';
		differM->mono = (uint8_t **) get_img(differM->width, differM->height, sizeof(uint8_t));

		struct pxm_img *differY = (struct pxm_img*) multialloc(sizeof(struct pxm_img), 1, 1);
		differY->height = halftoneC->height;
		differY->width = halftoneC->width;
		differY->pxm_type = 'g';
		differY->mono = (uint8_t **) get_img(differY->width, differY->height, sizeof(uint8_t));



		differC = findDifference(halftoneC, beforeC, differC);
		differM = findDifference(halftoneM, beforeM, differM);
		differY = findDifference(halftoneY, beforeY, differY);

		struct doubleImage *inputImageC2 = generateCTImage(differC);

		cpeC = calculateCpe(inputImageC2, differC, cpp);
		cpeM = calculateCpe(inputImageC2, differM, cpp);
		cpeY = calculateCpe(inputImageC2, differY, cpp);

		// Optimize overall uniform pattern

		for (int i = 0; i<10; i++){

			printf("Iteration %d : Jointly optimize C and Y patterns  \n", i+1);

			performCompleteDBSForScreenDesign(config,inputImageC2,halftoneC,cpeC,differC,cpeC,differY,cpeY,
					beforeC,beforeC,beforeC,cpp, 1);


			printf("Iteration %d :Jointly optimize M and Y patterns \n",i+1);
			performCompleteDBSForScreenDesign(config,inputImageC2,halftoneC,cpeC,differM,cpeM,differY,cpeY,
					beforeC,beforeC,beforeC,cpp, 1);

			printf("Iteration %d :Jointly optimize C and M patterns\n",i+1);
			performCompleteDBSForScreenDesign(config,inputImageC2,halftoneC,cpeC,differC,cpeC,differM,cpeM,
					beforeC,beforeC,beforeC,cpp, 1);
		}


		updateMatrix(halftoneC, matrixC, currentlevel, 1, beforeC);
		updateMatrix(halftoneM, matrixM, currentlevel, 1, beforeM);
		updateMatrix(halftoneY, matrixY, currentlevel, 1, beforeY);

		writeMatrix(matrixC, config->outputMatrixCPath);
		writeMatrix(matrixM, config->outputMatrixMPath);
		writeMatrix(matrixY, config->outputMatrixYPath);


		//FREE memories
		multifree((double *)inputImageC1->data,2);
		free(inputImageC1);

		multifree((double *)inputImageC2->data,2);
		free(inputImageC2);

		multifree((double *)cpeC->data,2);
		free(cpeC);
		multifree((double *)cpeM->data,2);
		free(cpeM);
		multifree((double *)cpeY->data,2);
		free(cpeY);


		free_pxm(differC);
		free_pxm(beforeC);

		free_pxm(differM);
		free_pxm(beforeM);

		free_pxm(differY);
		free_pxm(beforeY);
	}

	//-------------------------------------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Level by level design from 86->128

	for (unsigned int seqId = 1; seqId <= 43; seqId++){


		fprintf(stdout,"\n***********************************************************************************************************");
		fprintf(stdout, "\n \t\t\t  MATRIX size %u Processing order: 86->128 (Level %u)", config->MatrixSize, (seqId +85));
		fprintf(stdout,"\n*********************************************************************************************************\n");
		fflush(stdout);

		//current level
		double currentlevel = (double) (seqId + 85);
		int level = (int)currentlevel;

		struct pxm_img *beforeY =  samepattern(htY);
		struct pxm_img *beforeC =  samepattern(htC);
		struct pxm_img *beforeM =  samepattern(htM);

		double differ = 129 - currentlevel;

		double test61 =  countNum(htY);
		printf("before remove is %f\n ", test61);


		int Ytwdots = ceil (((config->MaxLevel)/2)+2);
		printf("Ytwdots is %d\n", Ytwdots );


		htY = removeDots(htY, differ, 2, config->MatrixSize,Ytwdots, 2);

		struct doubleImage *inputImageY = generateCTImage(htY);

		cpeY = calculateCpe(inputImageY, htY, cpp);

		performCompleteDBSForScreenDesign(config, inputImageY,htY,cpeY,htY,cpeY,
				htY, cpeY, beforeY,beforeY, beforeY, cpp, 2);


		struct pxm_img *differY = (struct pxm_img*) multialloc(sizeof(struct pxm_img), 1, 1);
		differY->height = htY->height;
		differY->width = htY->width;
		differY->pxm_type = 'g';
		differY->mono = (uint8_t **) get_img(differY->width, differY->height, sizeof(uint8_t));

		differY = findDifference(htY, beforeY, differY);

		double test6 =  countNum(differY);
		printf("The differY is %f\n ", test6);


		struct pxm_img *differC = (struct pxm_img*) multialloc(sizeof(struct pxm_img), 1, 1);
		differC->height = differY->height;
		differC->width = differY->width;
		differC->pxm_type = 'g';
		differC->mono = (uint8_t **) get_img(differC->width, differC->height, sizeof(uint8_t));

		struct pxm_img *differM = (struct pxm_img*) multialloc(sizeof(struct pxm_img), 1, 1);
		differM->height = differY->height;
		differM->width = differY->width;
		differM->pxm_type = 'g';
		differM->mono = (uint8_t **) get_img(differM->width, differM->height, sizeof(uint8_t));


		htC = mergePattern(differY,htC, 0.50);
		htM = mergePattern(differY,htM, 1);

		double test29 =  countNum(htC);
		printf("The htC is %f\n ", test29);
		double test28 =  countNum(htM);
		printf("The htM is %f\n ", test28);

		struct doubleImage *inputImageC = generateCTImage(htC);

		cpeC = calculateCpe(inputImageC, htC, cpp);
		cpeM = calculateCpe(inputImageC, htM, cpp);


		for (int i = 0; i<5; i++){

			printf("Iteration %d :Jointly optimize C and M patterns\n",i+1);
			performCompleteDBSForScreenDesign(config,inputImageC,htY,cpeY,htC,cpeC,htM,cpeM,
					beforeY, beforeC, beforeM, cpp, 3);
		}


		updateMatrix(htC, matrixC, currentlevel, 1, beforeC);
		updateMatrix(htM, matrixM, currentlevel, 1, beforeM);

		writeMatrix(matrixC, config->outputMatrixCPath);
		writeMatrix(matrixM, config->outputMatrixMPath);


		//FREE memories
		multifree((double *)inputImageY->data,2);
		free(inputImageY);

		multifree((double *)inputImageC->data,2);
		free(inputImageC);

		multifree((double *)cpeY->data,2);
		free(cpeY);
		multifree((double *)cpeC->data,2);
		free(cpeC);
		multifree((double *)cpeM->data,2);
		free(cpeM);

		free_pxm(differY);

		free_pxm(beforeC);
		free_pxm(beforeM);
		free_pxm(beforeY);
	}

	//-------------------------------------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Level by level design from 128->255
	printf("Part 3 of function");


	for (unsigned int seqId = 1; seqId <= 170; seqId++){

		fprintf(stdout,"\n***********************************************************************************************************");
		fprintf(stdout, "\n \t\t\t  MATRIX size %u Processing order: 128->255 (Level %u)", config->MatrixSize, (85 + seqId));
		fprintf(stdout,"\n*********************************************************************************************************\n");
		fflush(stdout);

		//current level
		double currentlevel = (double) (85 + seqId);
		int level = (int)currentlevel;
		double differ = currentlevel;

		struct pxm_img* beforeC =  samepattern(htC);
		struct pxm_img* beforeM =  samepattern(htM);
		struct pxm_img* beforeY =  samepattern(ht2Y);


		if (currentlevel<129){

			ht2Y = addDots (ht2Y,differ,2,config->MatrixSize, config->MaxLevel);
			printf("add dots\n");
			double count = countNum(ht2Y);
			printf("Y dots is %f\n", count);


			struct doubleImage *inputImageY = generateCTImage(ht2Y);
			struct doubleImage *cpeY = calculateCpe(inputImageY, ht2Y, cpp);

			performCompleteDBSForScreenDesign(config, inputImage,ht2Y,cpeY,ht2Y,cpeY,
					ht2Y, cpeY, beforeY,beforeY, beforeY, cpp, 2);

			updateMatrix(ht2Y, matrixY, currentlevel, 1, beforeY);
			writeMatrix(matrixY, config->outputMatrixYPath);


		}
		else{

			// C screen
			htC = addDots (htC,differ,2,config->MatrixSize, config->MaxLevel);
			printf("add dots\n");
			double count2 = countNum(htC);
			printf("C dots is %f\n", count2);
			struct doubleImage *inputImageC = generateCTImage(htC);
			struct doubleImage *cpeC = calculateCpe(inputImageC, htC, cpp);
			performCompleteDBSForScreenDesign(config, inputImage,htC,cpeC,htC,cpeC,htC, cpeC,
					beforeC,beforeC, beforeC,cpp, 2);
			updateMatrix(htC, matrixC, currentlevel, 1, beforeC);
			writeMatrix(matrixC, config->outputMatrixCPath);


			// M screen
			htM = addDots (htM,differ,2,config->MatrixSize, config->MaxLevel);
// 			printf("add dots\n");
			double count1 = countNum(htM);
// 			printf("M dots is %f\n", count1);
			struct doubleImage *inputImageM = generateCTImage(htM);
			struct doubleImage *cpeM = calculateCpe(inputImageM, htM, cpp);
			performCompleteDBSForScreenDesign(config, inputImage,htM,cpeM,htM,cpeM,htM, cpeM,
					beforeM,beforeM, beforeM, cpp, 2);
			updateMatrix(htM, matrixM, currentlevel, 1, beforeM);
			writeMatrix(matrixM, config->outputMatrixMPath);

			// Y screen
			ht2Y = addDots (ht2Y,differ,2,config->MatrixSize, config->MaxLevel);
// 			printf("add dots\n");
			double count = countNum(ht2Y);
// 			printf("Y dots is %f\n", count);
			struct doubleImage *inputImageY = generateCTImage(ht2Y);
			struct doubleImage *cpeY = calculateCpe(inputImageY, ht2Y, cpp);
			performCompleteDBSForScreenDesign(config, inputImage,ht2Y,cpeY,ht2Y,cpeY,
					ht2Y, cpeY, beforeY,beforeY, beforeY, cpp, 2);
			updateMatrix(ht2Y, matrixY, currentlevel, 1, beforeY);
			writeMatrix(matrixY, config->outputMatrixYPath);



			//FREE memories
			multifree((double *)inputImageY->data,2);
			free(inputImageY);

			multifree((double *)inputImageC->data,2);
			free(inputImageC);

			multifree((double *)inputImageM->data,2);
			free(inputImageM);

			multifree((double *)cpeY->data,2);
			free(cpeY);
			multifree((double *)cpeC->data,2);
			free(cpeC);
			multifree((double *)cpeM->data,2);
			free(cpeM);

			free_pxm(beforeC);
			free_pxm(beforeM);
			free_pxm(beforeY);


		}
	}
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------------------------------------

	// Clean up!!
	free(config);

	free_pxm(htC);
	free_pxm(htM);
	free_pxm(htY);
	free_pxm(ht2Y);


	deallocateShiftedImage(cpp);
	deallocateShiftedImage(psf);

	fflush(stdout);
	return(0);


	printf("******************************************************************************\n ");
	printf("Jointly level-by-level design C and M screens--Done! \n ");
	printf("******************************************************************************\n ");
}




// Allocates and populate the configuration options used in DBS Mono with default values.
Config* getConfigurations() {
	Config *config = malloc(sizeof(Config));

	config->inputImagePath = "../samples/CT-CMY-128.tif";
	config->inputImagePath2 = "../samples/CT-C-128.tif";
	config->initialHalftonePath = "../samples/CMY-initial-128.tif";

	config->MatrixSize = 128;
	config->MaxLevel = 255;

	config->outputMatrixCPath ="../out/128CMatrix.txt";
	config->outputMatrixMPath ="../out/128MMatrix.txt";
	config->outputMatrixYPath ="../out/128YMatrix.txt";


	config->scaleFactor = 3500;
	config->hvsSpreadSize = 23;

	config->enableToggle = 0;
	config->enableSwap = 1;
//	config->applyWrapAround = 0;

	config->gamma = 1.0;
	config->blockHeight = 1;
	config->blockWidth = 1;
	config->swapSize = 128;

	config->maxIterationCount = 200;
	config->minAcceptableChangeCount = 5;

	config->enableVerboseDebugging = 0;
	return config;
}

// De-allocates a matrix whose 0-index in the center. This currently is used for the human visual 
// system, and its autocorrelation.
void deallocateShiftedImage(struct doubleImage *image) {

	// Deoffset the image, so that it can be freed.
	for (int i = -image->borderSize; i <= image->borderSize; i++) {
		image->data[i] -= image->borderSize;
	}
	image->data -= image->borderSize;

	multifree((double*) image->data, 2);
	free(image);
}
