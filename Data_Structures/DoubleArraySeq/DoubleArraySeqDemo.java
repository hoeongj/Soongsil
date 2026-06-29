public class DoubleArraySeqDemo {
    public static void main(String[] args) {
        DoubleArraySeq seq1 = new DoubleArraySeq(); // 용량 10의 배열 생성
        seq1.addAfter(1.1); // 1.1
        seq1.addAfter(2.2); // 1.1 2.2
        seq1.addAfter(3.3); // 1.1 2.2 3.3
        seq1.addBefore(4.4); // 1.1 2.2 4.4 3.3
        seq1.start(); // 커서가 첫번째 위치로 감
        seq1.addAfter(5.5); // 1.1 5.5 2.2 4.4 3.3
        seq1.advance(); // 커서가 한칸 앞(2.2)로 감
        seq1.addAfter(6.6); // 1.1 5.5 2.2 6.6 4.4 3.3

        System.out.print("seq1 배열의 데이터 : ");
        seq1.printtest(seq1);

        DoubleArraySeq seq2 = new DoubleArraySeq(15); // 용량 15의 배열 생성

        System.out.println();
        System.out.println("seq2 배열의 크기 : "+seq2.getCapacity());

        seq2 = seq1.clone(); // seq2는 seq과 같은 배열이 됨

        seq1.ensureCapacity(20); // seq1의 배열의 크기는 20이 됨

        seq1.addAll(seq2);  //  seq1 = 1.1 5.5 2.2 6.6 4.4 3.3 1.1 5.5 2.2 6.6 4.4 3.3

        System.out.println("seq1의 크기 : "+seq1.getCapacity());
        System.out.print("seq1의 데이터 : ");
        seq1.printtest(seq1);

        DoubleArraySeq seq3 = new DoubleArraySeq();
        seq3 = seq3.concatenation(seq1,seq2);  // seq3은 seq1과 seq2가 합쳐진  1.1 5.5 2.2 6.6 4.4 3.3 1.1 5.5 2.2 6.6 4.4 3.3 1.1 5.5 2.2 6.6 4.4 3.3 가 됨
        System.out.println();
        System.out.print("Seq3의 데이터 : ");
        seq3.printtest(seq3);
        System.out.println();

        System.out.println("seq1의 현재 요소 : "+seq1.getCurrent());

        System.out.println("seq1의 커서가 있는지 확인 : "+seq1.isCurrent());

        seq1.removeCurrent();  // 현재 커서에 있는 요소(6.6)를 삭제
        System.out.print("seq1의 커서에 있는 요소 삭제 후 데이터 : ");
        seq1.printtest(seq1);

        System.out.println();
        System.out.println("seq1배열에 들어가 있는 요소 개수 : "+seq1.size());

        seq1.trimToSize();
        System.out.println("seq1배열에 실제로 들어가 있는 요소만 남긴 배열의 크기 : "+seq1.getCapacity());
    }
}