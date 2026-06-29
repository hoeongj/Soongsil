/**
 * 학번_20221528
 * 이름_홍성주
 *
 * DoubleArraySeq 클래스는 double 타입의 숫자 시퀀스를 저장하고 관리한다.
 * 이 프로젝트에서는 자바를 통해 Array를 구현한다.
 * 현재 요소를 지정할 수 있고 array에 있는 addAfter(), addBefore(), addAll(), advance(),
 * clone(), concatenation(), esureCapacity(), getCapacity(), getCurrent(), isCurrent(),
 * removeCurrent(), size(), start(), trimToSize()를 모두 제공한다.
 * 제한사항:
 * 시퀸스의 용량은 생성 후 변경할 수 있지만 최대 용량은 컴퓨터의 여유 메모리 양에 따라 제한됩니다.
 * 사용 가능한 메모리가 모두 소진되면 OutOfMemoryError를 발생시킵니다.
 * - 시퀸스의 용량은  2,147,483,647을 초과할 수 없습니다.
 */
class DoubleArraySeq implements Cloneable {
    private double[] data; // 데이터를 저장하는 배열
    private int manyItems; // 현재 저장된 요소 개수
    private int currentIndex; // 현재 선택된 요소의 인덱스 (없으면 -1)


    //DoubleArraySeq()가 비어있으면 초기 용량은 10이다.

    public DoubleArraySeq() {
        try {
            data = new double[10];
            manyItems = 0;
            currentIndex = -1;
        } catch (OutOfMemoryError e) {
            throw new OutOfMemoryError("이 시퀸스를 사용하기에 메모리 용량이 부족!");
        }
    }

    /**
     * 특정 크기의 빈 시퀀스를 생성하는 생성자.
     * @param initialCapacity 초기 용량 (용량은 음수가 나올 수 없음)
     * @throws IllegalArgumentException 초기 용량이 음수일 경우 예외 발생
     */
    public DoubleArraySeq(int initialCapacity) {
        if (initialCapacity < 0) throw new IllegalArgumentException("초기 용량은 음수가 될 수 없습니다.");
        try {
            data = new double[initialCapacity];
            manyItems = 0;
            currentIndex = -1;
        } catch (OutOfMemoryError e) {
            throw new OutOfMemoryError("이 시퀸스를 사용하기에 메모리 용량이 부족!");
        }
    }

    /**
     * 현재 요소 앞에 새 요소를 추가한다.
     * @param element 추가할 요소
     */
    public void addBefore(double element) {
        try {
            ensureCapacity(manyItems + 1);

            if (!isCurrent()) {
                currentIndex = 0;
            }

            for (int i = manyItems; i > currentIndex; i--) {
                data[i] = data[i - 1];
            }

            data[currentIndex] = element;
            manyItems++;
        } catch (OutOfMemoryError e) {
            throw new OutOfMemoryError("이 시퀸스를 사용하기에 메모리 용량이 부족!");
        }
    }

    /**
     * 현재 요소 뒤에 새 요소를 추가한다.
     * @param element 추가할 요소
     */
    public void addAfter(double element) {
        try {
            ensureCapacity(manyItems + 1);
            if (!isCurrent()) {
                currentIndex = manyItems; // 마지막 위치에서 추가
            } else {
                currentIndex++;
                for (int i = manyItems; i > currentIndex; i--) {
                    data[i] = data[i - 1]; // 기존 요소 오른쪽으로 이동
                }
            }
            data[currentIndex] = element;
            manyItems++;
        } catch (OutOfMemoryError e) {
            throw new OutOfMemoryError("이 시퀸스를 사용하기에 메모리 용량이 부족!");
        }
    }

    /**
     * 다른 DoubleArraySeq의 요소들을 현재 시퀸스 끝에 추가한다.
     * @param addend 추가할 시퀸스
     * @throws NullPointerException addend가 null이면 예외 발생시킨다.
     */
    public void addAll(DoubleArraySeq addend) {
        if (addend == null) throw new NullPointerException("addend가 null이면 안됩니다!");
        try {
            ensureCapacity(manyItems + addend.manyItems);
            System.arraycopy(addend.data, 0, data, manyItems, addend.manyItems);
            manyItems += addend.manyItems;
        } catch (OutOfMemoryError e) {
            throw new OutOfMemoryError("이 시퀸스를 사용하기에 메모리 용량이 부족!");
        }
    }

    /**
     * 현재 요소를 다음 요소로 이동시킨다.
     * @throws IllegalStateException 현재 요소가 없을 경우 예외 발생
     */
    public void advance() {
        if(isCurrent()) {
            if (currentIndex < manyItems - 1) {
                currentIndex++;
            } else {
                currentIndex = -1;
            }
        }
        else throw new IllegalStateException("현재 요소가 없습니다.");
    }


    /**
     * 현재 시퀸스를 복제한다.
     * @return 복제된 DoubleArraySeq 객체
     */
    public DoubleArraySeq clone() {
        try {
            DoubleArraySeq arrayClone = (DoubleArraySeq) super.clone();
            arrayClone.data = this.data.clone();
            return arrayClone;
        } catch (CloneNotSupportedException e) {
            throw new RuntimeException("이 클래스는 Cloneable을 구현하지 않음");
        }
    }

    /**
     * 입력받은 두개의 시퀸스를 연결하여 새로운 시퀸스를 만단다.
     * @param s1 첫 번째 시퀸스
     * @param s2 두 번째 시퀸스
     * @return 새로운 시퀸스
     */
    public static DoubleArraySeq concatenation(DoubleArraySeq s1, DoubleArraySeq s2) {
        if (s1 == null || s2 == null) throw new NullPointerException("s1 또는 s2가 null이면 안됩니다!");
        try {
            DoubleArraySeq s3 = new DoubleArraySeq(s1.manyItems + s2.manyItems);
            s3.addAll(s1);
            s3.addAll(s2);
            return s3;
        } catch (OutOfMemoryError e) {
            throw new OutOfMemoryError("이 시퀸스를 사용하기에 메모리 용량이 부족!");
        }
    }

    /**
     * 시퀸스의 크기를 설정한다.
     * @param minimumCapacity 최소 필요 용량
     */
    public void ensureCapacity(int minimumCapacity) {
        try {
            if (data.length >= minimumCapacity) return;
            double[] bigArray = new double[minimumCapacity];
            System.arraycopy(data, 0, bigArray, 0, manyItems);
            data = bigArray;
        }
        catch (OutOfMemoryError e) {
            throw new OutOfMemoryError("이 시퀸스를 사용하기에 메모리 용량이 부족!");
        }
    }
    /**
     * 시퀸스의 크기를 확인한다.
     * @return 시퀸스의 크기
     */
    public int getCapacity() {
        return data.length;
    }

    /**
     * 시퀸스의 현재 요소를 확인한다.
     * @return 시킌스의 현재 요소
     * @throws IllegalStateException 현재 요소가 없을 경우 예외 발생
     */

    public double getCurrent() {
        if (!isCurrent()) throw new IllegalStateException("현재 요소가 없음");
        return data[currentIndex];
    }

    /**
     * 시퀸스의 현재 요소가 있는지 확인한다.
     * @return 요가사 있으면 true 없으면 false
     */

    public boolean isCurrent() {
        if(currentIndex >= 0 && currentIndex < manyItems) return true;
        else return false;
    }

    /**
     * 현재 요소를 삭제한다.
     * @throws IllegalStateException 현재 요소가 없을 경우 예외 발생
     */
    public void removeCurrent() {
        if (!isCurrent()) throw new IllegalStateException("현재 요소가 없음");

        for (int i = currentIndex; i < manyItems - 1; i++) {
            data[i] = data[i + 1];
        }
        manyItems--;

        if (manyItems == 0) {
            currentIndex = -1;
        } else if (currentIndex >= manyItems) {
            currentIndex = manyItems - 1;
        }
    }

    /**
     * 현재 배열에 몇개의 요소가 들어있는지 확인한다.
     */
    public int size() {
        return manyItems;
    }

    /**
     * 현재 요소를 첫 번째 요소로 설정한다.
     */
    public void start() {
        currentIndex = 0;
    }

    /**
     * 현재 배열의 크기를 실제로 요소가 들어있는 수로 만든다.
     */
    public void trimToSize() {
        try {
            if (data.length > manyItems) {
                double[] newArray = new double[manyItems];
                System.arraycopy(data, 0, newArray, 0, manyItems);
                data = newArray;
            }
        }
        catch (OutOfMemoryError e) {
            throw new OutOfMemoryError("이 시퀸스를 사용하기에 메모리 용량이 부족!");
        }
    }
    public void printtest(DoubleArraySeq seq){
        for(int i = 0 ; i < seq.size() ; i++) System.out.print(seq.data[i]+" ");
    }
    public static void main(String[] args){
    }
}