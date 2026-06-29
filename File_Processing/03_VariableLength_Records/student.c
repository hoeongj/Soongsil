#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "student.h"

static int readHeader(FILE *fp, int *num_records, int *list_head)
{
	if(fseek(fp, 0, SEEK_SET) != 0) return 0;
	if(fread(num_records, sizeof(int), 1, fp) != 1) return 0;
	if(fread(list_head, sizeof(int), 1, fp) != 1) return 0;
	return 1;
}

static int writeHeader(FILE *fp, int num_records, int list_head)
{
	if(fseek(fp, 0, SEEK_SET) != 0) return 0;
	if(fwrite(&num_records, sizeof(int), 1, fp) != 1) return 0;
	if(fwrite(&list_head, sizeof(int), 1, fp) != 1) return 0;
	return 1;
}

static int makeRecordData(char *data, int data_size, char *id, char *name, char *dept, char *addr, char *email)
{
	int len = snprintf(data, data_size, "%s#%s#%s#%s#%s#", id, name, dept, addr, email);

	if(len < 0 || len >= data_size) return -1;
	if(len + (int)sizeof(short) > MAX_RECORD_SIZE) return -1;

	return len;
}

static void copyField(const char *buf, int *pos, char *dest, int max_len)
{
	int j = 0;

	while(buf[*pos] != '#' && buf[*pos] != '\0')
	{
		if(j < max_len)
		{
			dest[j++] = buf[*pos];
		}
		(*pos)++;
	}

	dest[j] = '\0';
	if(buf[*pos] == '#') (*pos)++;
}

static void parseRecord(const char *buf, STUDENT *s)
{
	int pos = 0;

	copyField(buf, &pos, s->sid, sizeof(s->sid) - 1);
	copyField(buf, &pos, s->name, sizeof(s->name) - 1);
	copyField(buf, &pos, s->dept, sizeof(s->dept) - 1);
	copyField(buf, &pos, s->addr, sizeof(s->addr) - 1);
	copyField(buf, &pos, s->email, sizeof(s->email) - 1);
}

static const char *getFieldValue(const STUDENT *s, enum FIELD f)
{
	switch(f)
	{
		case SID: return s->sid;
		case NAME: return s->name;
		case DEPT: return s->dept;
		case ADDR: return s->addr;
		case EMAIL: return s->email;
		default: return NULL;
	}
}

static int parseCondition(char *condition, char *field_name, int field_name_size, char *field_value, int field_value_size)
{
	char *eq = strchr(condition, '=');
	int name_len;
	int value_len;

	if(eq == NULL) return 0;

	name_len = (int)(eq - condition);
	value_len = (int)strlen(eq + 1);
	if(name_len <= 0 || name_len >= field_name_size) return 0;
	if(value_len >= field_value_size) return 0;

	memcpy(field_name, condition, name_len);
	field_name[name_len] = '\0';
	memcpy(field_value, eq + 1, value_len + 1);

	return 1;
}

void print(const STUDENT *s[], int n);

int insert(FILE *fp, char *id, char *name, char *dept, char *addr, char *email)
{
	char data[MAX_RECORD_SIZE];
	int data_len;
	short new_size;
	int num_records;
	int list_head;
	int prev_offset = -1;
	int curr_offset;
	int found_offset = -1;
	int found_next = -1;

	data_len = makeRecordData(data, sizeof(data), id, name, dept, addr, email);
	if(data_len < 0) return 0;

	new_size = (short)(sizeof(short) + data_len);
	if(!readHeader(fp, &num_records, &list_head)) return 0;

	curr_offset = list_head;
	while(curr_offset != -1)
	{
		short slot_len;
		char marker;
		int next_offset;

		if(fseek(fp, curr_offset, SEEK_SET) != 0) return 0;
		if(fread(&slot_len, sizeof(short), 1, fp) != 1) return 0;
		if(fread(&marker, sizeof(char), 1, fp) != 1) return 0;
		if(fread(&next_offset, sizeof(int), 1, fp) != 1) return 0;

		if(marker == '*' && slot_len >= new_size)
		{
			found_offset = curr_offset;
			found_next = next_offset;
			break;
		}

		prev_offset = curr_offset;
		curr_offset = next_offset;
	}

	if(found_offset != -1)
	{
		if(fseek(fp, found_offset + sizeof(short), SEEK_SET) != 0) return 0;
		if(fwrite(data, sizeof(char), data_len, fp) != (size_t)data_len) return 0;

		if(prev_offset == -1)
		{
			list_head = found_next;
		}
		else
		{
			if(fseek(fp, prev_offset + sizeof(short) + sizeof(char), SEEK_SET) != 0) return 0;
			if(fwrite(&found_next, sizeof(int), 1, fp) != 1) return 0;
		}
	}
	else
	{
		if(fseek(fp, 0, SEEK_END) != 0) return 0;
		if(fwrite(&new_size, sizeof(short), 1, fp) != 1) return 0;
		if(fwrite(data, sizeof(char), data_len, fp) != (size_t)data_len) return 0;
	}

	num_records++;
	if(!writeHeader(fp, num_records, list_head)) return 0;

	return 1;
}

int delete(FILE *fp, enum FIELD f, char *keyval)
{
	int num_records;
	int list_head;
	long curr_offset = HEADER_SIZE;

	if(f != SID) return 0;
	if(!readHeader(fp, &num_records, &list_head)) return 0;

	while(1)
	{
		short rec_len;
		int data_len;
		char data[MAX_RECORD_SIZE];
		STUDENT s;
		long rec_start = curr_offset;

		if(fseek(fp, rec_start, SEEK_SET) != 0) break;
		if(fread(&rec_len, sizeof(short), 1, fp) != 1) break;
		if(rec_len <= (short)sizeof(short) || rec_len > MAX_RECORD_SIZE) break;

		data_len = rec_len - (int)sizeof(short);
		if(fread(data, sizeof(char), data_len, fp) != (size_t)data_len) break;
		data[data_len] = '\0';

		if(data[0] != '*')
		{
			parseRecord(data, &s);
			if(strcmp(s.sid, keyval) == 0)
			{
				char marker = '*';

				if(fseek(fp, rec_start + sizeof(short), SEEK_SET) != 0) return 0;
				if(fwrite(&marker, sizeof(char), 1, fp) != 1) return 0;
				if(fwrite(&list_head, sizeof(int), 1, fp) != 1) return 0;

				list_head = (int)rec_start;
				num_records--;
				if(!writeHeader(fp, num_records, list_head)) return 0;

				return 1;
			}
		}

		curr_offset += rec_len;
	}

	return 0;
}

void search(FILE *fp, enum FIELD f, char *keyval)
{
	STUDENT *students = NULL;
	const STUDENT **results = NULL;
	int capacity = 0;
	int count = 0;
	long curr_offset = HEADER_SIZE;

	while(1)
	{
		short rec_len;
		int data_len;
		char data[MAX_RECORD_SIZE];
		STUDENT s;
		const char *field_value;

		if(fseek(fp, curr_offset, SEEK_SET) != 0) break;
		if(fread(&rec_len, sizeof(short), 1, fp) != 1) break;
		if(rec_len <= (short)sizeof(short) || rec_len > MAX_RECORD_SIZE) break;

		data_len = rec_len - (int)sizeof(short);
		if(fread(data, sizeof(char), data_len, fp) != (size_t)data_len) break;
		data[data_len] = '\0';

		if(data[0] != '*')
		{
			parseRecord(data, &s);
			field_value = getFieldValue(&s, f);
			if(field_value != NULL && strcmp(field_value, keyval) == 0)
			{
				if(count == capacity)
				{
					int new_capacity = (capacity == 0) ? 16 : capacity * 2;
					STUDENT *new_students = (STUDENT *)realloc(students, sizeof(STUDENT) * new_capacity);

					if(new_students == NULL)
					{
						free(students);
						print(NULL, 0);
						return;
					}

					students = new_students;
					capacity = new_capacity;
				}

				students[count++] = s;
			}
		}

		curr_offset += rec_len;
	}

	if(count > 0)
	{
		results = (const STUDENT **)malloc(sizeof(STUDENT *) * count);
		if(results == NULL)
		{
			free(students);
			print(NULL, 0);
			return;
		}

		for(int i = 0; i < count; i++)
		{
			results[i] = &students[i];
		}
	}

	print(results, count);
	free(results);
	free(students);
}

enum FIELD getFieldID(char *fieldname)
{
	if(strcmp(fieldname, "SID") == 0) return SID;
	if(strcmp(fieldname, "NAME") == 0) return NAME;
	if(strcmp(fieldname, "DEPT") == 0) return DEPT;
	if(strcmp(fieldname, "ADDR") == 0) return ADDR;
	if(strcmp(fieldname, "EMAIL") == 0) return EMAIL;
	return (enum FIELD)(-1);
}

int main(int argc, char *argv[])
{
	FILE *fp;

	if(argc < 3) return 0;

	if(strcmp(argv[1], "-i") == 0)
	{
		if(argc != 8) return 0;

		fp = fopen(argv[2], "r+b");
		if(fp == NULL)
		{
			int zero = 0;
			int neg_one = -1;

			fp = fopen(argv[2], "w+b");
			if(fp == NULL) return 0;
			if(fwrite(&zero, sizeof(int), 1, fp) != 1)
			{
				fclose(fp);
				return 0;
			}
			if(fwrite(&neg_one, sizeof(int), 1, fp) != 1)
			{
				fclose(fp);
				return 0;
			}
		}

		insert(fp, argv[3], argv[4], argv[5], argv[6], argv[7]);
		fclose(fp);
	}
	else if(strcmp(argv[1], "-d") == 0)
	{
		char field_name[16];
		char field_value[MAX_RECORD_SIZE];

		if(argc != 4) return 0;
		fp = fopen(argv[2], "r+b");
		if(fp == NULL) return 0;
		if(!parseCondition(argv[3], field_name, sizeof(field_name), field_value, sizeof(field_value)))
		{
			fclose(fp);
			return 0;
		}

		delete(fp, getFieldID(field_name), field_value);
		fclose(fp);
	}
	else if(strcmp(argv[1], "-s") == 0)
	{
		char field_name[16];
		char field_value[MAX_RECORD_SIZE];

		if(argc != 4) return 0;
		fp = fopen(argv[2], "rb");
		if(fp == NULL) return 0;
		if(!parseCondition(argv[3], field_name, sizeof(field_name), field_value, sizeof(field_value)))
		{
			fclose(fp);
			return 0;
		}

		search(fp, getFieldID(field_name), field_value);
		fclose(fp);
	}

	return 0;
}

void print(const STUDENT *s[], int n)
{
	printf("#records=%d\n", n);
	for(int i = 0; i < n; i++)
	{
		printf("%s#%s#%s#%s#%s#\n", s[i]->sid, s[i]->name, s[i]->dept, s[i]->addr, s[i]->email);
	}
}
