/*
 * mime.c - mime.types process - handles lookups from file extensions to content-type
 * author: larkinsb@rhodes.edu
 *
 */
#include "httpserver.h"


/*
 * init_mimetypes - builds extension / mime-type lookup table
 *
 */
void init_mimetypes(void) {
  FILE *mtypes;
  char line[1024];
  char *type, *ep, *ext, *cur;
  int i, typelen, estart;

  // open the mapping file
  mtypes = fopen("mime.types", "r");


  // loop over each line in the file
  while (fgets(line, 1024, mtypes) != NULL) {
    if ((line[0] == '#') || (line[0] == '\n'))
      continue;

    ext = NULL;
    cur = line;

    // find end of MIME type part
    for (i=0; ((cur[i] != ' ') && (cur[i] != '\t') && (cur[i] != '\n')); i++)
      ;
    typelen = i;

    // skip whitespace
    while ((cur[i] == ' ') || (cur[i] == '\t')) {
      i++;
    }

    // if no extensions match type, skip this entry
    if (cur[i] == '\n') {
      continue;
    }

    // build one entry for each extension listed in the file

    estart = i;     // we are at the start of the first extension
    // file extensions are defined for this type (at least one)
    while (cur[i] != '\n') {
      if ((cur[i] == ' ') || (cur[i] == '\t')) {
        // found whitespace after extension, multiple extensions
        mimetypes[ecount].type = strndup(line, typelen);
        mimetypes[ecount].ext  = strndup(&cur[estart], i-estart);
        ecount++;
        estart = -1;
      } else if (estart == -1) {
        estart = i;
      }
      i++;
    }

    // add the entry
    mimetypes[ecount].type = strndup(line, typelen);
    mimetypes[ecount].ext  = strndup(&cur[estart], i-estart);
    ecount++;
  }

  fclose(mtypes);
}



/*
 * search_mimetypes - looks up a content-type from a file extension
 *
 * @param ext string containing the file extension (no .)
 * @returns string containing the content-type or NULL if not found
 *
 */
char *search_mimetypes(char *ext) {
  // loop over files
  for (int i=0; i<ecount; i++) {
    if (!strcasecmp(ext, mimetypes[i].ext))
      return mimetypes[i].type;
  }
  return NULL;
}
