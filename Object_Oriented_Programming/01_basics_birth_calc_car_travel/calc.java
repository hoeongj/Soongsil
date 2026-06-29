import java.util.Scanner;
public class calc {
    public static void main(String[] args) {
        Scanner s = new Scanner(System.in);
        System.out.print("연산 입력>>");
        double n1 = s.nextDouble();
        String math = s.next();
        double n2 = s.nextDouble();

        double result = switch (math) {
            case "더하기" -> { yield n1 + n2;}
            case "빼기" -> { yield n1 - n2; }
            case "곱하기" -> { yield n1 * n2; }
            case "나누기" -> { yield n1 / n2; }
            default -> {
                System.out.print("사칙연산이 아닙니다.");
                System.exit(0);
                yield 0.0;
            }
        };
        System.out.println(n1+" "+math+" "+n2+"의 계산 결과는 "+result);
        s.close();
    }
}