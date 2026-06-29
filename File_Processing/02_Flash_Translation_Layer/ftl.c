// 주의사항
// 1. sectormapping.h에 정의되어 있는 상수 변수를 우선적으로 사용해야 함
// (예를 들면, PAGES_PER_BLOCK의 상수값은 채점 시에 변경할 수 있으므로 반드시 이 상수 변수를 사용해야 함)
// 2. sectormapping.h에 필요한 상수 변수가 정의되어 있지 않을 경우 본인이 이 파일에서 만들어서 사용하면 됨
// 3. 새로운 data structure가 필요하면 이 파일에서 정의해서 쓰기 바람(sectormapping.h에 추가하면 안됨)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "sectormapping.h"

extern FILE *flashmemoryfp;

extern int fdd_read(int ppn, char *pagebuf);
extern int fdd_write(int ppn, char *pagebuf);
extern int fdd_erase(int pbn);

#define NUM_PAGES	(BLOCKS_PER_DEVICE * PAGES_PER_BLOCK)

static int mapping_table[DATAPAGES_PER_DEVICE];
static int free_bitmap[NUM_PAGES];
static int garbage_bitmap[NUM_PAGES];
static int block_pbn;

static int reads;
static int writes;
static int erases;

static int alloc_free_page(void);
static int garbage_collection_func(void);

//
// flash memory를 처음 사용할 때 필요한 초기화 작업, 예를 들면 address mapping table에 대한
// 초기화 등의 작업을 수행한다. 따라서, 첫 번째 ftl_write() 또는 ftl_read()가 호출되기 전에
// file system에 의해 반드시 먼저 호출이 되어야 한다.
//
void ftl_open()
{
	int i;

	// address mapping table 생성 및 초기화
	// (모든 lsn -> psn = -1)
	for (i = 0; i < DATAPAGES_PER_DEVICE; i++) mapping_table[i] = -1;

	// free page bitmap 생성 및 초기화
	// (모든 비트 1)
	// garbage page bitmap 생성 및 초기화
	// (모든 비트 0)
	for (i = 0; i < NUM_PAGES; i++) {
		free_bitmap[i] = 1;
		garbage_bitmap[i] = 0;
	}

	// 그 이외 필요한 작업 수행

	// free block 초기 pbn = BLOCKS_PER_DEVICE - 1
	block_pbn = BLOCKS_PER_DEVICE - 1;

	reads = 0;
	writes = 0;
	erases = 0;

	return;
}

// free block을 제외하고 첫 번째 free page를 찾아서 할당함.
// 성공 시 할당된 psn를 return, 실패 시 -1를 return.

static int alloc_free_page(void)
{
	int i;
	int start_block = block_pbn * PAGES_PER_BLOCK;
	int end_block = start_block + PAGES_PER_BLOCK;

	for (i = 0; i < NUM_PAGES; i++) {
		if (i >= start_block && i < end_block) {
			continue;
		}
		if (free_bitmap[i] == 1) {
			free_bitmap[i] = 0;
			return i;
		}
	}

	return -1;
}

// garbage page를 이용하여 free page를 만들고, 거기서 첫 번째를 현재 write를 위해 할당함.
// 성공 시 할당된 psn을 return, 실패 시 -1 return.
static int garbage_collection_func(void)
{
	int i, j;
	int victim_block = -1;
	int victim_start;
	int old_block = block_pbn * PAGES_PER_BLOCK;
	int alloc_sector = -1;
	char pagebuf[PAGE_SIZE];

	// free block을 제외하고 첫 번째 garbage page를 찾는다
	for (i = 0; i < NUM_PAGES; i++) {
		if (i >= old_block && i < old_block + PAGES_PER_BLOCK) {
			continue;
		}
		if (garbage_bitmap[i] == 1) {
			victim_block = i / PAGES_PER_BLOCK;
			break;
		}
	}

	if (victim_block == -1) {
		return -1;
	}

	victim_start = victim_block * PAGES_PER_BLOCK;

	// victim block의 유효 페이지(garbage나 free가 아닌 페이지)를 free block의 같은 위치에 복사
	for (j = 0; j < PAGES_PER_BLOCK; j++) {
		int victim_sector = victim_start + j;
		int dest_sector = old_block + j;

		// 유효 페이지는 garbage도 아니고 free(아직 안쓴)도 아님
		if (garbage_bitmap[victim_sector] == 0 && free_bitmap[victim_sector] == 0) {
			int spare_sector;

			fdd_read(victim_sector, pagebuf);
			reads++;

			fdd_write(dest_sector, pagebuf);
			writes++;

			// spare에 저장된 lsn을 읽어 mapping table 갱신함
			memcpy(&spare_sector, pagebuf + SECTOR_SIZE, sizeof(int));
			mapping_table[spare_sector] = dest_sector;

			// 예전 free block에서 이 위치는 이제 사용됨
			free_bitmap[dest_sector] = 0;
		}
	}

	// victim block을 erase
	fdd_erase(victim_block);
	erases++;

	// erase된 victim block은 이제 새로운 free block: bitmap 정리
	for (j = 0; j < PAGES_PER_BLOCK; j++) {
		garbage_bitmap[victim_start + j] = 0;
		free_bitmap[victim_start + j] = 1;
	}

	// 옛 free block에서 데이터가 복사되지 않은 위치들 = 새로 생긴 free page들
	// 그 중 첫 번째를 현재 write를 위해 할당, 나머지는 free page bitmap에 그대로 1 유지
	for (j = 0; j < PAGES_PER_BLOCK; j++) {
		int psn = old_block + j;
		if (free_bitmap[psn] == 1) {
			if (alloc_sector == -1) {
				alloc_sector = psn;
				free_bitmap[psn] = 0;
			}
			// 나머지는 1로 유지 (free page bitmap에 추가됨)
		}
	}

	// free block을 erase된 블록(victim)으로 갱신
	block_pbn = victim_block;

	return alloc_sector;
}

//
// 이 함수를 호출하는 쪽(file system)에서 이미 sectorbuf가 가리키는 곳에 512B의 메모리가 할당되어 있어야 함
// (즉, 이 함수에서 메모리를 할당 받으면 안됨)
//
void ftl_read(int lsn, char *sectorbuf)
{
	int psn;
	char pagebuf[PAGE_SIZE];

	if (lsn < 0 || lsn >= DATAPAGES_PER_DEVICE) {
		return;
	}

	psn = mapping_table[lsn];
	if (psn == -1) {
		return;
	}

	fdd_read(psn, pagebuf);
	reads++;

	memcpy(sectorbuf, pagebuf, SECTOR_SIZE);

	return;
}

//
// 이 함수를 호출하는 쪽(file system)에서 이미 sectorbuf가 가리키는 곳에 512B의 메모리가 할당되어 있어야 함
// (즉, 이 함수에서 메모리를 할당 받으면 안됨)
//
void ftl_write(int lsn, char *sectorbuf)
{
	int new_psn;
	int old_psn;
	char pagebuf[PAGE_SIZE];

	if (lsn < 0 || lsn >= DATAPAGES_PER_DEVICE) {
		return;
	}

	// overwrite 발생 시 기존 psn은 garbage 처리
	old_psn = mapping_table[lsn];
	if (old_psn != -1) {
		garbage_bitmap[old_psn] = 1;
	}

	// free page 할당 시도
	new_psn = alloc_free_page();
	if (new_psn == -1) {
		// free page가 더 이상 없으면 garbage page를 이용해 free page 생성
		new_psn = garbage_collection_func();
		if (new_psn == -1) {
			return;
		}
	}

	// pagebuf 준비: sector 영역 + spare 영역의 맨왼쪽 4B에 lsn (binary integer)
	memset(pagebuf, 0xFF, PAGE_SIZE);
	memcpy(pagebuf, sectorbuf, SECTOR_SIZE);
	memcpy(pagebuf + SECTOR_SIZE, &lsn, sizeof(int));

	fdd_write(new_psn, pagebuf);
	writes++;

	mapping_table[lsn] = new_psn;

	return;
}

//
// Address mapping table 등을 출력하는 함수이며, 출력 포맷은 과제 설명서 참조
// 출력 포맷을 반드시 지켜야 하며, 그렇지 않는 경우 채점시 불이익을 받을 수 있음
//
void ftl_print()
{
	int i;

	printf("lsn psn\n");
	for (i = 0; i < DATAPAGES_PER_DEVICE; i++) {
		printf("%d %d\n", i, mapping_table[i]);
	}
	printf("free block=%d\n", block_pbn);
	printf("#reads=%d #writes=%d #erases=%d\n", reads, writes, erases);

	return;
}
