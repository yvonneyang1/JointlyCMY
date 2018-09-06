#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parseXml.h"

void parseStory (xmlDocPtr doc,xmlNodePtr cur, struct Config * config){
	xmlChar* key;
	cur = cur->xmlChildrenNode;
	while (cur !=NULL){
		if((!xmlStrcmp(cur->name,(const xmlChar *)"infn"))){

			  			key = xmlNodeListGetString(doc,cur->xmlChildrenNode,1);
			  			printf("infn: %s\n",(char *)key);
			  			config->infn = (char *)key;
			  			printf("config->infn: %s\n",config->infn);
			  			/*xmlFree(key);*/
		}
			  		if((!xmlStrcmp(cur->name,(const xmlChar *)"output_fname"))){
			  			key = xmlNodeListGetString(doc,cur->xmlChildrenNode,1);
			  			printf("output_fname: %s\n",key);
			  			config->output_fname = (char *)key;
			  			/*xmlFree(key);*/
			  		}
			  		if((!xmlStrcmp(cur->name,(const xmlChar *)"scale_factor"))){
			  			key = xmlNodeListGetString(doc,cur->xmlChildrenNode,1);
			  			printf("scale_factor: %f\n",atof(key));
			  			config->scale_factor = atoi(key);
			  			xmlFree(key);
			  		}
			  		if((!xmlStrcmp(cur->name,(const xmlChar *)"checkflag"))){
			  			key = xmlNodeListGetString(doc,cur->xmlChildrenNode,1);
			  			printf("checkflag: %f\n",atof(key));
			  			config->checkflag = atoi(key);
			  			xmlFree(key);
			  		}
			  		if((!xmlStrcmp(cur->name,(const xmlChar *)"Rand_Init"))){
			  			key = xmlNodeListGetString(doc,cur->xmlChildrenNode,1);
			  			printf("Rand_Init: %f\n",atof(key));
			  			config->Rand_Init = atoi(key);
			  			xmlFree(key);
			  		}
			  		if((!xmlStrcmp(cur->name,(const xmlChar *)"initHfname"))){
			  			key = xmlNodeListGetString(doc,cur->xmlChildrenNode,1);
			  			printf("initHfname: %s\n",key);
			  			config->initHfname = (char *)key;
			  			/*xmlFree(key);*/
			  		}
			  		if((!xmlStrcmp(cur->name,(const xmlChar *)"tog_flag"))){
			  			key = xmlNodeListGetString(doc,cur->xmlChildrenNode,1);
			  			printf("tog_flag: %f\n",atof(key));
			  			config->tog_flag = atoi(key);
			  			xmlFree(key);
			  		}
			  		if((!xmlStrcmp(cur->name,(const xmlChar *)"swap_flag"))){
			  			key = xmlNodeListGetString(doc,cur->xmlChildrenNode,1);
			  			printf("swap_flag: %f\n",atof(key));
			  			config->swap_flag = atoi(key);
			  			xmlFree(key);
			  		}
			  		if((!xmlStrcmp(cur->name,(const xmlChar *)"gamma"))){
			  			key = xmlNodeListGetString(doc,cur->xmlChildrenNode,1);
			  			printf("gamma: %f\n",atof(key));
			  			config->gamma = atof(key);
			  			xmlFree(key);
			  		}
			  		if((!xmlStrcmp(cur->name,(const xmlChar *)"size3b"))){
			  			key = xmlNodeListGetString(doc,cur->xmlChildrenNode,1);
			  			printf("size3b: %f\n",atof(key));
			  			config->size3b = atoi(key);
			  			xmlFree(key);
			  		}



		cur = cur->next;
	}
	return;
}

static void parseDoc(char *docname) {
	xmlDocPtr doc;
	xmlNodePtr cur;
	xmlChar* key;
	doc = xmlParseFile(docname);

	if (doc == NULL){
		fprintf(stderr,"Document not parsed successfully. \n");
		return;
	}

	cur = xmlDocGetRootElement(doc);

	if (cur == NULL){
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return;
	}

	/*cur=cur->xmlChildrenNode;
	while (cur !=NULL){
		key = xmlNodeListGetString(doc, cur,1);
		fprintf("keyword:%s\n",key);
		xmlFree(key);

		cur=cur->next;


	}
*/
	if (xmlStrcmp(cur->name,(const xmlChar*)"inputs")){
		fprintf(stderr,"document of the wrong structure.\n");
		xmlFreeDoc(doc);
		return;
	}

	cur=cur->xmlChildrenNode;
	/*while(cur !=NULL){
		if((!xmlStrcmp(cur->name,(const xmlChar*)"list"))){
			parseStory(doc,cur);

		}
		cur=cur->next;
	}*/

	xmlFreeDoc(doc);
	return;

}

/*int main(int argc, char **argv){
	char *docname;

	if(argc<=1){
		printf("Usage: %s docname\n", argv[0]);
		return(0);
	}

	docname = argv[1];
	parseDoc (docname);
	return(1);
}*/
