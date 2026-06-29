import java.util.Scanner;

abstract class Shape {
    private Shape next;
    public Shape() { next = null; }
    public void setNext(Shape obj) { next = obj; } // 링크 연결
    public Shape getNext() { return next; }
    public abstract void draw(); // 추상 메소드
}

class Line extends Shape {
    public void draw() {
        System.out.println("Line");
    }
}

class Rect extends Shape {
    public void draw() {
        System.out.println("Rect");
    }
}

class Circle extends Shape {
    public void draw() {
        System.out.println("Circle");
    }
}


class GraphicEditor {
    private Shape head = null;
    private Shape tail = null;
    private Scanner sc = new Scanner(System.in);

    public void insert() {
        System.out.print("Line(1), Rect(2), Circle(3)>>");
        int sel = sc.nextInt();
        Shape shape = null;

        switch (sel) {
            case 1: shape = new Line(); break;
            case 2: shape = new Rect(); break;
            case 3: shape = new Circle(); break;
            default:
                System.out.println("잘못된 입력!!");
                return;
        }

        if(head == null) {
            head = shape;
            tail = shape;
        }
        else{
            tail.setNext(shape);
            tail = shape;
        }
    }

    public void delete() {
        System.out.print("삭제할 도형의 위치>>");
        int del = sc.nextInt();

        if(head == null) {
            System.out.println("삭제할 수 없습니다.");
            return;
        }

        if(del == 1) {
            head = head.getNext();
            if(head == null) tail = null;
            return;
        }

        Shape one = head;
        Shape two = head.getNext();
        int index = 2;

        while (two != null && index < del) {
            one = two;
            two = two.getNext();
            index++;
        }

        if(two == null) System.out.println("삭제할 수 없습니다.");
            else{
            one.setNext(two.getNext());
            if(two == tail) tail = one;
        }
    }

    public void showAll() {
        Shape tmp = head;
        while (tmp != null) {
            tmp.draw();
            tmp = tmp.getNext();
        }
    }
}

public class pro14 {
    public static void main(String[] args) {
        GraphicEditor ed = new GraphicEditor();
        Scanner sc = new Scanner(System.in);

        System.out.println("그래픽 에디터 Beauty Graphic Editor를 실행합니다.");

        while(true) {
            System.out.print("삽입(1), 삭제(2), 모두 보기(3), 종료(4)>>");
            int choice = sc.nextInt();

            switch (choice) {
                case 1:
                    ed.insert();
                    break;
                case 2:
                    ed.delete();
                    break;
                case 3:
                    ed.showAll();
                    break;
                case 4:
                    System.out.print("Beauty Graphic Editor를 종료합니다.");
                    return;
                default:
                    System.out.println("잘못된 입력!!");
            }
        }
    }
}