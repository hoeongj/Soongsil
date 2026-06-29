class Book{
    String author;
    String bookName;
    String buyer;

    Book(String author, String bookName, String buyer){
        this.author = author;
        this.bookName = bookName;
        this.buyer = buyer;
    }

    @Override
    public boolean equals(Object obj){
        Book nb = (Book) obj;
        if(nb.author == this.author && nb.bookName == this.bookName) return true;
        return false;
    }

    @Override
    public String toString(){
        return buyer + "이 구입한 도서: " + author + "의 " + bookName;
    }
}

public class pro2_BookApp{
    public static void main(String[] args){
        Book a = new Book("황기태", "중고자바", "김하진");
        Book b = new Book("황기태", "명품자바", "하여린");
        System.out.println(a);
        System.out.println(b);
        if(a.equals(b)) System.out.println("같은 책");
        else System.out.println("다른 책");
    }
}