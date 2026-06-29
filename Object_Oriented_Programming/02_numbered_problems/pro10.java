import java.util.Scanner;

public class pro10 {
    public static void main(String[] args) {
        int[][] input = new int[4][4];

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                input[i][j] = (int)(Math.random() * 256);
            }
        }

        System.out.println("4×4 배열에 랜덤한 값을 저장한 후 출력합니다.");
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                System.out.print(input[i][j]+" ");
            }
            System.out.println();
        }

        Scanner scanner = new Scanner(System.in);
        System.out.print("임계값 입력>>");
        int input2 = scanner.nextInt();

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (input[i][j] > input2) {
                    input[i][j] = 255;
                } else {
                    input[i][j] = 0;
                }
            }
        }

        System.out.println("임계값 처리 후 배열:");
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                System.out.print(input[i][j]+" ");
            }
            System.out.println();
        }

        scanner.close();
    }
}