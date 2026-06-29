import java.util.Scanner;

public class pro14 {
    public static void main(String[] args) {
        Scanner sc = new Scanner(System.in);
        System.out.println("***** 갬블링 게임을 시작합니다. *****");
        String go = "yes";
        while (go.equals("yes")) {
            int[] ran = new int[3];
            System.out.print("엔터키 입력>>");
            sc.nextLine();
            for (int i = 0; i < 3; i++) ran[i] = (int) (Math.random() * 3);

            for (int i = 0; i < ran.length; i++) System.out.print(ran[i] + " ");
            System.out.println();

            if (ran[0] == ran[1] && ran[1] == ran[2]) {
                System.out.println("성공! 대박났어요!");
                System.out.print("계속하시겠습니까?(yes/no)>>");
                go = sc.nextLine();
            }
        }
        System.out.print("게임을 종료합니다.");
        sc.close();
    }
}