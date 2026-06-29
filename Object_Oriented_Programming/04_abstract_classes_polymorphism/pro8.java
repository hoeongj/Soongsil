import java.util.Scanner;

abstract class Box {
    protected int size; // 현재 박스에 들어 있는 재료의 양
    public Box(int size) { // 생성자
        this.size = size;
    }
    public boolean isEmpty() {
        return size == 0;
    } // 박스가 빈 경우 true 리턴
    public abstract boolean consume(); // 박스에 들어 있는 재료를 일정량 소비
    public abstract void print(); // 박스에 들어 있는 량을 "*" 문자로 출력
}

class IngredientBox extends Box{
    String Box_name;
    public IngredientBox(String name, int size){
        super(size);
        Box_name = name;
    }
    public boolean consume(){
        if(isEmpty()){
            System.out.println("원료가 부족합니다.");
            return false;
        }
        size--;
        return true;
    }

    public void print(){
        System.out.println(Box_name+" *****"+size);
    }
}

 class pro8 {
    public static void main(String[] args){
        Scanner sc = new Scanner(System.in);
        System.out.println("*****청춘 커피 자판기 입니다.*****");
        IngredientBox coffee = new IngredientBox("커피",5);
        IngredientBox prim = new IngredientBox("프림",5);
        IngredientBox sugar = new IngredientBox("설탕",5);
        int n = 0;
        while(n != 4){
            coffee.print();
            prim.print();
            sugar.print();
            System.out.print("다방커피:1, 설탕 커피:2, 블랙 커피:3, 종료:4>>");
            n = sc.nextInt();
            switch (n){
                case 1:
                    if (coffee.isEmpty() || prim.isEmpty() || sugar.isEmpty()) {
                        System.out.println("원료가 부족합니다.");
                    } else {
                        coffee.consume();
                        prim.consume();
                        sugar.consume();
                    }
                    break;
                case 2:
                    if (coffee.isEmpty() || sugar.isEmpty()) {
                        System.out.println("원료가 부족합니다.");
                    } else {
                        coffee.consume();
                        sugar.consume();
                    }
                    break;
                case 3:
                    if (coffee.isEmpty()) {
                        System.out.println("원료가 부족합니다.");
                    } else {
                        coffee.consume();
                    }
                    break;
            }
        }
        System.out.print("청춘 커피 자판기 프로그램을 종료합니다");
    }
}