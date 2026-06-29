class VArray {
    int size = 0;
    int capacity;
    int[] arr;

    VArray(int a) {
        capacity = a;
        arr = new int[a];
    }

    int capacity() {
        return capacity;
    }

    int size() {
        return size;
    }

    int add(int in) {
        if (size == capacity) encapacity();
        arr[size] = in;
        size++;
        return in;
    }

    void encapacity() {
        int[] newarr = new int[capacity * 2];
        for (int i = 0; i < size; i++) {
            newarr[i] = arr[i];
        }
        arr = newarr;
        capacity *= 2;
    }

    void insert(int index, int in) {
        if (size == capacity) encapacity();
        for (int i = size - 1; i >= index; i--) arr[i + 1] = arr[i];
        arr[index] = in;
        size++;
    }

    void printAll() {
        for (int i = 0; i < size; i++) System.out.print(arr[i] + " ");
        System.out.println();
    }

    void remove(int index) {
        if(index >= size) return;
        for (int i = index; i < size - 1; i++) arr[i] = arr[i + 1];
        size--;
    }
}
public class pro14 {
    public static void main(String[] args) {
        VArray v = new VArray(5); // 5개 정수를 저장하는 가변 배열 객체 생성
        System.out.println("용량: " + v.capacity() + ", 저장된 개수: " + v.size());

        for (int i = 0; i < 7; i++) // 7개 저장
            v.add(i); // 배열에 순서대로 정수 i 값 저장
        System.out.println("용량: " + v.capacity() + ", 저장된 개수: " + v.size());
        v.printAll();


        v.insert(3, 100); // 배열의 인덱스 3에 100 삽입
        v.insert(5, 200); // 배열의 인덱스 5에 200 삽입
        System.out.println("용량: " + v.capacity() + ", 저장된 개수: " + v.size());
        v.printAll();

        v.remove(10); // 배열의 인덱스 10의 정수 삭제
        System.out.println("용량: " + v.capacity() + ", 저장된 개수: " + v.size());
        v.printAll();

        for (int i = 50; i < 55; i++) // 5개 저장
            v.add(i); // 배열에 순서대로 정수 i 값 저장
        System.out.println("용량: " + v.capacity() + ", 저장된 개수: " + v.size());
        v.printAll();
    }
}