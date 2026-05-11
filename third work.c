#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct FileNode {
    char *name;             // 文件/目录名
    int isDir;              // 1:目录 0:文件
    struct FileNode *firstChild;  // 左孩子：第一个子项
    struct FileNode *nextSibling; // 右兄弟：下一个同层项
} FileNode;

// 创建新节点
FileNode *createNode(const char *name, int isDir) {
    FileNode *node = (FileNode *)malloc(sizeof(FileNode));
    if (!node) return NULL;
    
    node->name = strdup(name);
    node->isDir = isDir;
    node->firstChild = NULL;
    node->nextSibling = NULL;
    return node;
}

// 节点比较函数（用于排序）
int cmpNode(const void *a, const void *b) {
    const FileNode *nodeA = *(const FileNode **)a;
    const FileNode *nodeB = *(const FileNode **)b;
    return strcmp(nodeA->name, nodeB->name);
}

// 递归构建目录树
FileNode *buildTree(const char *path) {
    struct stat statBuf;
    if (stat(path, &statBuf) != 0 || !S_ISDIR(statBuf.st_mode)) {
        fprintf(stderr, "Error: Invalid directory path\n");
        return NULL;
    }

    DIR *dir = opendir(path);
    if (!dir) return NULL;

    FileNode *root = createNode(path, 1);
    if (!root) {
        closedir(dir);
        return NULL;
    }

    struct dirent *entry;
    FileNode **children = NULL;
    int childCount = 0;

    // 收集子节点
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

        if (stat(fullPath, &statBuf) != 0) continue;

        FileNode *child = createNode(entry->d_name, S_ISDIR(statBuf.st_mode));
        if (!child) continue;

        if (child->isDir) {
            child = buildTree(fullPath);
            if (!child) continue;
        }

        children = realloc(children, (childCount + 1) * sizeof(FileNode *));
        children[childCount++] = child;
    }
    closedir(dir);

    // 排序子节点（可选）
    if (childCount > 1) {
        qsort(children, childCount, sizeof(FileNode *), cmpNode);
    }

    // 构建兄弟链表
    if (childCount > 0) {
        root->firstChild = children[0];
        for (int i = 1; i < childCount; i++) {
            children[i-1]->nextSibling = children[i];
        }
    }

    free(children);
    return root;
}

// 树形输出
void printTree(FileNode *node, const char *prefix, int isLast) {
    if (!node) return;

    printf("%s", prefix);
    printf(isLast ? "`-- " : "|-- ");
    printf("%s", node->name);
    if (node->isDir) printf("/");
    printf("\n");

    // 构建新的前缀
    char newPrefix[256];
    snprintf(newPrefix, sizeof(newPrefix), "%s%s", prefix, isLast ? "    " : "|   ");

    // 递归输出子节点
    FileNode *child = node->firstChild;
    while (child) {
        printTree(child, newPrefix, child->nextSibling == NULL);
        child = child->nextSibling;
    }
}

// 统计节点总数
int countNodes(FileNode *root) {
    if (!root) return 0;
    return 1 + countNodes(root->firstChild) + countNodes(root->nextSibling);
}

// 统计叶子节点数
int countLeaves(FileNode *root) {
    if (!root) return 0;
    if (!root->firstChild) return 1 + countLeaves(root->nextSibling);
    return countLeaves(root->firstChild) + countLeaves(root->nextSibling);
}

// 计算树高度
int treeHeight(FileNode *root) {
    if (!root) return 0;
    int leftHeight = treeHeight(root->firstChild);
    int rightHeight = treeHeight(root->nextSibling);
    return 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);
}

// 统计目录和文件数
void countDirFile(FileNode *root, int *dirs, int *files) {
    if (!root) return;
    
    if (root->isDir) (*dirs)++;
    else (*files)++;
    
    countDirFile(root->firstChild, dirs, files);
    countDirFile(root->nextSibling, dirs, files);
}

// 释放整棵树
void freeTree(FileNode *root) {
    if (!root) return;
    freeTree(root->firstChild);
    freeTree(root->nextSibling);
    free(root->name);
    free(root);
}

int main(int argc, char *argv[]) {
    const char *path = argc > 1 ? argv[1] : ".";
    
    FileNode *root = buildTree(path);
    if (!root) return 1;

    // 输出树形结构
    printf("%s/\n", root->name);
    FileNode *child = root->firstChild;
    while (child) {
        printTree(child, "", child->nextSibling == NULL);
        child = child->nextSibling;
    }

    // 统计信息
    int dirs = 0, files = 0;
    countDirFile(root, &dirs, &files);
    printf("\n%d个目录, %d个文件\n", dirs, files);
    printf("二叉树结点总数: %d\n", countNodes(root));
    printf("叶子结点数: %d\n", countLeaves(root));
    printf("树的高度: %d\n", treeHeight(root));

    freeTree(root);
    return 0;
}