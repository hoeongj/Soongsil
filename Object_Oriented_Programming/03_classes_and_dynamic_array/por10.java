import java.util.Scanner;
public class por10 {
    public static void main(String[] args) {
        int select = 0;
        int date;
        int n;
        String text;
        String[] diary = new String[30];
        for(int i = 0; i < diary.length; i++) diary[i] = "...";
        Scanner sc = new Scanner(System.in);

        System.out.println("****** 2024년 10월 다이어리 ******");
        while(true){
            try{
            System.out.print("기록:1, 보기:2, 종료:3>>");
            select = sc.nextInt();
            if(select == 1){
                System.out.print("날짜(1~30)와 텍스트(빈칸없이 4글자이하)>>");
                date = sc.nextInt();
                text = sc.next();
                diary[date-1] = text;
            }

            else if(select == 2){
                for(int i = 0; i < diary.length; i++){
                    if(diary[i].equals("...")) System.out.print(diary[i] + "      ");
                    else{
                        System.out.print(diary[i]);
                        n = diary[i].length();
                        switch(n){
                            case 1: System.out.print(" ");
                            case 2: System.out.print(" ");
                            case 3: System.out.print(" ");
                            case 4: System.out.print("   "); break;
                        }
                    }
                    if((i+1) % 7 == 0) System.out.println();
                }
                System.out.println();
            }

            else if(select == 3){
                System.out.print("종료 되었습니다.");
                break;
            }

            else throw new Exception();
        }
            catch (Exception e){
                System.out.println("잘못된 값 입력!!");
                sc.nextLine();
            }
        }

    }
}