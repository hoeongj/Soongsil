import java.util.Random;
import java.util.Scanner;

// 네 가지 정렬 알고리즘(삽입/선택/병합/퀵)을 직접 구현하고, 각 알고리즘이
// 배열을 정렬해 나가는 중간 단계를 한 줄씩 출력하여 동작 과정을 시각적으로 보여 준다.
public class sort {

    // insertionSort: 앞쪽부터 정렬된 영역에 현재 값을 적절한 위치에 삽입
    public static void insertionSort(int[] arr) {
        int index;
        int temp;
        for (int i = 1; i < arr.length; i++) {
            index = i;
            for (int j = i - 1; j >= 0; j--) {
                // 앞의 값들 중에서 현재 값보다 큰 값 찾기
                if (arr[i] < arr[j]) index = j;
                else break;
            }
            temp = arr[i];
            if (index != i) {
                // index부터 오른쪽으로 한 칸씩 밀고, arr[i] 삽입
                System.arraycopy(arr, index, arr, index + 1, i - index);
                arr[index] = temp;
                // 정렬 단계 출력
                for (int k = 0; k < arr.length; k++) System.out.print(arr[k] + " ");
                System.out.println();
            }
        }
        print(arr); // 최종 결과 출력
    }

    // selectionSort: 가장 큰 값을 찾아 뒤쪽부터 채워나감
    public static void selectionSort(int[] arr) {
        int big;
        int temp;
        for (int i = arr.length - 1; i > 0; i--) {
            big = 0;
            for (int j = 1; j <= i; j++) {
                if (arr[j] > arr[big]) {
                    big = j;
                }
            }
            // 현재 i 위치에 가장 큰 값을 넣음
            temp = arr[i];
            arr[i] = arr[big];
            arr[big] = temp;
            // 정렬 단계 출력
            for (int k = 0; k < arr.length; k++) System.out.print(arr[k] + " ");
            System.out.println();
        }
        print(arr); // 최종 결과 출력
    }

    // mergeSort: 외부에서 호출되는 정렬 함수
    public static void mergeSort(int[] arr) {
        mergeSort2(arr, 0, arr.length); // 내부 재귀 함수 호출
        print(arr); // 최종 결과 출력
    }

    // mergeSort 재귀 함수
    private static void mergeSort2(int[] arr, int first, int n) {
        int n1, n2;
        if (n > 1) {
            n1 = n / 2;
            n2 = n - n1;
            // 배열을 반으로 나누어 각각 정렬
            mergeSort2(arr, first, n1);
            mergeSort2(arr, first + n1, n2);
            // 정렬된 두 배열 병합
            merge(arr, first, n1, n2);
            // 병합 단계 출력
            for (int i = first; i < first + n; i++) System.out.print(arr[i] + " ");
            System.out.println();
        }
    }

    // merge 함수: 두 정렬된 구간을 하나로 합침
    private static void merge(int[] arr, int first, int n1, int n2) {
        int[] temp = new int[n1 + n2];
        int cp = 0, cp1 = 0, cp2 = 0;
        while (cp1 < n1 && cp2 < n2) {
            if (arr[first + cp1] <= arr[first + n1 + cp2]) temp[cp++] = arr[first + cp1++];
            else temp[cp++] = arr[first + n1 + cp2++];
        }
        while (cp1 < n1) temp[cp++] = arr[first + cp1++];
        while (cp2 < n2) temp[cp++] = arr[first + n1 + cp2++];
        // 병합 결과를 원래 배열로 복사
        for (int i = 0; i < n1 + n2; i++) arr[first + i] = temp[i];
    }

    // quicksort: 외부에서 호출되는 정렬 함수
    public static void quicksort(int[] arr) {
        quicksort(arr, 0, arr.length); // 내부 재귀 함수 호출
        print(arr); // 최종 결과 출력
    }

    // quicksort 재귀 함수
    private static void quicksort(int[] arr, int first, int n) {
        int pivotIndex, n1, n2;
        if (n > 1) {
            pivotIndex = arrayDivid(arr, first, n); // 분할 기준 인덱스
            n1 = pivotIndex - first;
            n2 = n - n1 - 1;
            quicksort(arr, first, n1);            // 왼쪽 정렬
            quicksort(arr, pivotIndex + 1, n2);   // 오른쪽 정렬
            // 분할 후 단계 출력
            for (int i = first; i < first + n; i++) System.out.print(arr[i] + " ");
            System.out.println();
        }
    }

    // arrayDivid 함수 (배열을 pivot 기준으로 둘로 나눔)
    private static int arrayDivid(int[] arr, int first, int n) {
        int pivot = arr[first]; // pivot: 배열의 첫 번째 값
        int tooBigIndex = first + 1;
        int tooSmallIndex = first + n - 1;
        // 두 포인터가 만나기 전까지 반복
        while (tooBigIndex <= tooSmallIndex) {
            while (tooBigIndex <= tooSmallIndex && arr[tooBigIndex] <= pivot) tooBigIndex++;
            while (arr[tooSmallIndex] > pivot) tooSmallIndex--;
            if (tooBigIndex < tooSmallIndex) {
                // 큰 값과 작은 값 교환
                int temp = arr[tooBigIndex];
                arr[tooBigIndex] = arr[tooSmallIndex];
                arr[tooSmallIndex] = temp;
            }
        }
        // pivot을 올바른 위치로 이동
        arr[first] = arr[tooSmallIndex];
        arr[tooSmallIndex] = pivot;
        return tooSmallIndex; // pivot 위치 반환
    }

    // 정렬 완료 후 출력 함수
    public static void print(int[] arr) {
        System.out.print("정렬 완료 -> [");
        for (int i = 0; i < arr.length; i++) System.out.print(arr[i] + " ");
        System.out.println("]\n");
    }

    // 메인 함수: 정렬 테스트 프로그램
    public static void main(String args[]) {
        Scanner sc = new Scanner(System.in);
        Random ran = new Random();
        int[] arr = new int[32];  // 원본 배열
        int[] cArr = new int[32]; // 정렬용 복사 배열
        // 배열에 0~31 사이의 정수 32개 랜덤 저장
        for (int i = 0; i < arr.length; i++) arr[i] = ran.nextInt(32);
        System.out.println("<랜덤 배열 정렬 프로그램>\n");
        int n = 0;
        while (n < 5) {
            // 배열 출력
            System.out.print("생성된 랜덤 배열 : [ ");
            for (int i = 0; i < arr.length; i++) System.out.print(arr[i] + " ");
            System.out.println("]");
            // 메뉴 출력
            System.out.print("1.Insertion Sort 2.Selection Sort 3.Merge Sort 4.Quick Sort 5.quit 입력 : ");
            n = sc.nextInt();
            // 원본 배열을 복사하여 정렬
            System.arraycopy(arr, 0, cArr, 0, arr.length);
            switch (n) {
                case 1:
                    insertionSort(cArr);
                    break;
                case 2:
                    selectionSort(cArr);
                    break;
                case 3:
                    mergeSort(cArr);
                    break;
                case 4:
                    quicksort(cArr);
                    break;
            }
        }
        System.out.print("\n배열 정렬 프로그램을 종료합니다.");
        sc.close();
    }
}
