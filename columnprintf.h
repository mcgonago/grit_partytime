#ifndef __COLUMNPRINTF_H__
#define __COLUMNPRINTF_H__

#define OUTPUT_MODE_CONVERT (1)
#define OUTPUT_MODE_COLUMNS (2)

extern void ColumnPrintf(CLI_PARSE_INFO *pInfo, char *str);
extern void PrintTable(CLI_PARSE_INFO *pInfo);
extern void ColumnStore(CLI_PARSE_INFO *pInfo, char *str);
extern void ColumnReset(CLI_PARSE_INFO *pInfo);
extern void RemoveTrailingAdd2Inline(char *in);
extern void RemoveTrailing(char *out, char *in);
extern void ReplaceTabs(char *out, char *in);
extern void AddSpace(char *out, char *in);
extern void RemoveSpaceAtEnd(char *ptr);

extern void FixLine(CLI_PARSE_INFO *pInfo, char *out, char *in, char *team);
extern void FixTeamName(CLI_PARSE_INFO *pInfo, char *out, char *in);


#endif /* __COLUMNPRINTF_H__ */
