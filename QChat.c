#define FUSE_USE_VERSION 31
#define debug

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

#ifdef debug
    char * debugLog;
#endif

static const int maxNameLength = 10;

static struct qNode{
    char * name;
    char * content;
    int specie;                 // 0 for directory entry ; 1 for normal file
    struct qNode * list_ptr;    // record subfiles
    struct qNode * father;
    struct qNode * next;        // for linkedlist
    struct qNode * prev;

    struct qNode * userNext;    // used for user list
    struct qNode * userPrev;
};

static struct qNode * root;

/*
    Alloc new Node
*/
static struct qNode* malloc_node(){
    void * ptr = malloc(sizeof(struct qNode));
    memset(ptr,0,sizeof(struct qNode));
    return (struct qNode*) ptr;
}

static void set_node(struct qNode * node, char * _name, char * _content, int _specie, 
                struct qNode * p1, struct qNode * p2, struct qNode * p3, struct qNode * p4){
    node->name = _name;
    node->content = _content;
    node->specie = _specie;
    node->list_ptr = p1; node->next = p2; node->prev = p3; node->father = p4;
}

/*
    Avoid memory leak.
*/
static void free_node(struct qNode * node){
    if(node->content != NULL) free(node->content);
    if(node->name != NULL) free(node->name);
    struct qNode * tmpPtr = node->list_ptr;
    while(tmpPtr != NULL){
        struct qNode * tmpTmpPtr = tmpPtr;
        tmpPtr = tmpPtr->next;
        free_node(tmpTmpPtr);
    }
    free(node);
}

/*
    Find specific qNode from the root of the hierachy
*/
static int find_node(struct qNode** result, const struct qNode* _root, const char * path){
    int stringPtr = 1;
    while(path[stringPtr] != '\0'){
        int isFound = 0; // 0 for not found; 1 found;
        int isDir = 0; // 0 for common file; 1 for dir
        int tmp = 0;
        char tmpName[maxNameLength];
        while(path[stringPtr] != '/' && path[stringPtr] != '\0'){
            tmpName[tmp++] = path[stringPtr++];
        }
        tmpName[tmp] = '\0';
        if(path[stringPtr] == '/'){
            stringPtr++;
            isDir = 1;
        }
        struct qNode * nodePtr = _root->list_ptr;
        while(nodePtr != NULL){
            if(strcmp(nodePtr->name,tmpName) == 0){
                if((isDir == 1) && (nodePtr->specie == 1)) return -ENOENT;
                _root = nodePtr;
                isFound = 1;
                break;
            }
            nodePtr = nodePtr->next;
        }
        if(isFound == 0) return -ENOENT;
    }
    (*result) = _root;
    return 0;
}

/*
    Add new node in the subfiles.
*/
static void add_node(struct qNode * origin, struct qNode * new){
    new->next = origin->list_ptr;
    if(origin->list_ptr != NULL){
        origin->list_ptr->prev = new;
    }
    origin->list_ptr = new;
}

/*
    Remove a node in the list ,corresponding to father
*/
static void remove_node(struct qNode * target){
    struct qNode * father = target->father;
    if(father->list_ptr == target){
        assert(target->prev == NULL);
        father->list_ptr = target->next;
    }
    if(target->next != NULL) target->next->prev = target->prev;
    if(target->prev != NULL) target->prev->next = target->next;
    free_node(target);
}

/*
    Given : /Alice/Bob/t
    Return: /Alice
*/
static char * getHeadPath(const char * rawPath){
    int tmpPtr = 1;
    while(rawPath[tmpPtr] != '/' && rawPath[tmpPtr] != '\0') tmpPtr++;
    char * new = (char*) malloc(tmpPtr+4);
    int newPtr = 0;
    while(newPtr < tmpPtr){
        new[newPtr] = rawPath[newPtr];
        newPtr++;
    }
    new[newPtr] = '\0';
    return new;
}

static char * getTailPath(const char * rawPath){
    int size = strlen(rawPath);
    int length = size - 1;
    while(rawPath[length] != '/') length--;
    char * new = (char*) malloc(size-length+1);
    int tmpPtr = 0;
    while(length < size) new[tmpPtr++] = rawPath[length++];
    new[tmpPtr] = '\0';
    return new;
}

/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
static struct options {
	int show_help;
} options;

#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
	OPTION("-h", show_help),
	OPTION("--help", show_help),
	FUSE_OPT_END
};

static void *qchat_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	(void) conn;
	cfg->kernel_cache = 0;
	return NULL;
}

static int qchat_getattr(const char *path, struct stat *stbuf,
			 struct fuse_file_info *fi)
{
	(void) fi;
	memset(stbuf, 0, sizeof(struct stat));

    struct qNode * target;
    int res = find_node(&target, root, path);
    if(res != 0) return res;
    if(target->specie == 0){
        stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
    }else{
        stbuf->st_mode = S_IFREG | 0770;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(target->content);
    }
	return 0;
}

static int qchat_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags)
{
	(void) offset;
	(void) fi;
	(void) flags;

    struct qNode * target;
    int res = find_node(&target, root, path);
    if(res != 0) return res;

	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);
    target = target->list_ptr;
    while(target != NULL){
        filler(buf, target->name, NULL, 0, 0);
        target = target->next;
    }
	return 0;
}

static int qchat_open(const char *path, struct fuse_file_info *fi)
{
    struct qNode * target;
    int res = find_node(&target, root, path);
    if(res != 0) return -ENOENT;
	return 0;
}

static int qchat_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
    struct qNode * target;
    int res = find_node(&target, root, path);
    if(res != 0) return -ENOENT;

	len = strlen(target->content);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, target->content + offset, size);
	} else
		size = 0;

	return size;
}

static int qchat_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi){
    (void) fi;
    
    struct qNode * target;
    int res = find_node(&target, root, path); // ENOTSUP
    if(res != 0) return res;

    char * sender = getHeadPath(path);

    #ifdef debug
        strcat(debugLog,"raw sender :  ");
        strcat(debugLog,strdup(sender));
    #endif

    int senderLength = strlen(sender);
    sender[0] = '['; sender[senderLength] = ']'; sender[senderLength+1] = '\n'; sender[senderLength+2] = '\0';
    senderLength += 2;

    #ifdef debug
        strcat(debugLog,"\npipe sender :  ");
        strcat(debugLog,strdup(sender));
    #endif

    char * raw = target->content;
    int finalSize = (offset + size + senderLength > strlen(raw)) ? (offset + size + senderLength) : strlen(raw);
    char * newData = (char *) malloc(finalSize+1);
    memcpy(newData, raw, strlen(raw));
    memcpy(newData + offset, sender, senderLength);
    memcpy(newData + offset + senderLength, buf, size);
    newData[finalSize] = '\0';
    free(raw);
    free(sender);
    target->content = newData;

    // try to write to symmetry entry
    char * receiverPath = getTailPath(path);
    res = find_node(&target, root, receiverPath);
    
    #ifdef debug
        strcat(debugLog,"The receiver is :  ");
        strcat(debugLog,strdup(receiverPath));
    #endif

    free(receiverPath);

    if(res != 0 || target->specie == 1) return size;
    char * receiverName = getHeadPath(path);
    int nameLength = strlen(receiverName);
    for(int i = 0;i < nameLength;++i) receiverName[i] = receiverName[i+1];

    #ifdef debug
        strcat(debugLog,"\n The sender is : ");
        strcat(debugLog,strdup(receiverName));
        strcat(debugLog,"\n");
    #endif
    
    while(target != NULL){
        if(strcmp(target->name,receiverName) == 0) break;
        target = target->userNext;
    }
    free(receiverName);
    if(target == NULL) return size;

    assert(target->specie == 1);
    free(target->content);
    target->content = strdup(newData);

    #ifdef debug
        strcat(debugLog," Successfully add ");
        strcat(debugLog, strdup(buf));
        strcat(debugLog,"\n-----\n");
    #endif

    return size;
}

static int qchat_mkdir(const char *path, mode_t mode)
{
	int res;
    int length = strlen(path);
    int ptr = length - 1;
    while(path[ptr] != '/') ptr--;

    char newPath[length+1];
    int tmpCount = 0;
    while(tmpCount <= ptr){
        newPath[tmpCount] = path[tmpCount];
        tmpCount++;
    }
    newPath[tmpCount] = '\0';

    struct qNode * target;
    res = find_node(&target, root, newPath);
    if(res != 0) return res;

    struct qNode * new = malloc_node();
    tmpCount = 0;
    ptr++;
    while(path[ptr] != '\0') newPath[tmpCount++] = path[ptr++];
    newPath[tmpCount] = '\0';
    set_node(new, strdup(newPath), NULL, 0, NULL, NULL, NULL, target);
    add_node(target, new);
	return 0;
}

static int qchat_mknod(const char * path, mode_t mode, dev_t di){
    (void) di;
    (void) mode;
    int res = qchat_mkdir(path, mode);
    if(res != 0) return res;
    
    struct qNode * target;
    res = find_node(&target, root,path);
    if(res != 0) return -EPIPE;
    target->specie = 1;
    target->content = strdup("|---Welcome to QChat!---|\n");
    
    struct qNode * user;
    char * headPath = getHeadPath(path);
    res = find_node(&user, root, headPath);
    if(res != 0) return -EPIPE;
    assert(user->specie == 0);
    target->userNext = user->userNext;
    target->userPrev = user;
    if(user->userNext != NULL) user->userNext->userPrev = target;
    user->userNext = target;
    free(headPath);
    return 0;
}

static int qchat_unlink(const char * path){
    struct qNode * target;
    int res = find_node(&target, root, path);
    if(res != 0) return res;

    assert(target->userPrev != NULL);
    target->userPrev->userNext = target->userNext;
    if(target->userNext != NULL) target->userNext->userPrev = target->userPrev;

    remove_node(target);
    return 0;
}

static int qchat_rmdir(const char * path){
    struct qNode * target;
    int res = find_node(&target, root, path);
    if(res != 0) return res;
    if(target->list_ptr != NULL) return -EPIPE; // A Dirctory must delete after all subfiles is deleted. 
    remove_node(target);
    return 0;
}

static int qchat_release(const char *path, struct fuse_file_info *fi){
    return 0;
}

static int qchat_utimens(const char * path, const struct timespec tv[2],
			 struct fuse_file_info *fi){
    return 0;
}

static const struct fuse_operations qchat_oper = {
    .init       =   qchat_init,
    .getattr    =   qchat_getattr,
    .readdir    =   qchat_readdir,
    .open       =   qchat_open,
    .read       =   qchat_read,
    .mkdir      =   qchat_mkdir,
    .mknod      =   qchat_mknod,
    .release    =   qchat_release,
    .write      =   qchat_write,
    .utimens    =   qchat_utimens,
    .unlink     =   qchat_unlink,
    .rmdir      =   qchat_rmdir,
};

static void show_help(const char *progname)
{
	printf("usage: %s [options] <mountpoint>\n\n", progname);
	printf("File-system specific options:\n"
	       "    --help          get useful information\n"
	       "\n");
}

int main(int argc, char *argv[])
{
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	/* Parse options */
	if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;

	/* When --help is specified, first print our own file-system
	   specific help text, then signal fuse_main to show
	   additional help (by adding `--help` to the options again)
	   without usage: line (by setting argv[0] to the empty
	   string) */
	if (options.show_help) {
		show_help(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0][0] = '\0';
	}

    root = malloc_node();
    set_node(root,NULL,NULL,0,NULL,NULL,NULL,NULL);

    #ifdef debug
    debugLog = (char*)malloc(1500);
    struct qNode * tmp = malloc_node();
    set_node(tmp,strdup("log"),debugLog,1,NULL,NULL,NULL,root);
    root->list_ptr = tmp;
    #endif

	ret = fuse_main(args.argc, args.argv, &qchat_oper, NULL);
	fuse_opt_free_args(&args);
    free_node(root);
	return ret;
}
