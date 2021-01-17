#ifndef __COLUMNPRINTF_H__
#define __COLUMNPRINTF_H__

extern void ColumnPrintf(CLI_PARSE_INFO *pInfo, char *str);
extern void PrintTable(CLI_PARSE_INFO *pInfo);
extern void ColumnStore(CLI_PARSE_INFO *pInfo, char *str);
extern void ColumnReset(CLI_PARSE_INFO *pInfo);
extern void RemoveTrailingAdd2Inline(char *in);
extern void RemoveTrailing(char *out, char *in);
extern void ReplaceTabs(char *out, char *in);
extern void AddSpace(char *out, char *in);
extern void RemoveSpaceAtEnd(char *ptr);


#endif /* __COLUMNPRINTF_H__ */
