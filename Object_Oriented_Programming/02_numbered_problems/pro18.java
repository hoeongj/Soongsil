import java.util.Scanner;
import java.util.InputMismatchException;

public class pro18 {
    public static void main(String[] args) {
        Scanner sc = new Scanner(System.in);
        int[][] students = new int[10][2];

        System.out.println("10명 학생의 학번과 점수 입력");
        for (int i = 0; i < students.length; i++) {
            System.out.print((i+1) + ">>");
            students[i][0] = sc.nextInt();
            students[i][1] = sc.nextInt();
        }

        while (true) {
            System.out.print("학번으로 검색: 1, 점수로 검색: 2, 끝내려면 3>>");
            int choice = 0;

            try {
                choice = sc.nextInt();
            } catch (InputMismatchException e) {
                System.out.println("경고!! 정수를 입력하세요.");
                sc.nextLine();
                continue;
            }

            if (choice == 3) {
                System.out.print("프로그램을 종료합니다.");
                break;
            }

            if (choice == 1) {
                int studentId = 0;
                boolean realInput = false;
                while (!realInput) {
                    System.out.print("학번>>");
                    try {
                        studentId = sc.nextInt();
                        realInput = true;
                    } catch (InputMismatchException e) {
                        System.out.println("경고!! 정수를 입력하세요.");
                        sc.nextLine();
                    }
                }

                boolean realstudent = false;
                for (int i = 0; i < students.length; i++) {
                    if (students[i][0] == studentId) {
                        System.out.println(students[i][1] + "점");
                        realstudent = true;
                        break;
                    }
                }

                if (!realstudent) {
                    System.out.println(studentId + "의 학생은 없습니다.");
                }
            }

            else if (choice == 2) {
                int searchScore = 0;
                boolean rightInput = false;
                while (!rightInput) {
                    System.out.print("점수>> ");
                    try {
                        searchScore = sc.nextInt();
                        rightInput = true;
                    } catch (InputMismatchException e) {
                        System.out.println("경고!! 정수를 입력하세요.");
                        sc.nextLine();
                    }
                }
                boolean realStudent2 = false;
                System.out.print("점수가 " + searchScore + "인 학생은 ");
                for (int i = 0 ; i < students.length ; i++) {
                    if (students[i][1] == searchScore) {
                        System.out.print(students[i][0] + " ");
                        realStudent2 = true;
                    }
                }
                if (!realStudent2) {
                    System.out.println("없습니다.");
                }
                else System.out.println("입니다.");
            }
        }
    }
}