let handAnim = 0;
let mouthAnim = 0;
let isRecording = false;

function setup() {
  createCanvas(600, 400);
}

function draw() {
  background(240);

  if (keyIsDown(83) && isRecording == false) {
    saveGif('chara_pose_10s', 10);
    isRecording = true;
  }

  if (keyIsDown(70)) {
    fill(255, 255, 255, 200); noStroke();
    circle(random(0, 600), random(0, 400), random(100, 300));
    circle(random(0, 600), random(0, 400), random(100, 300));
  }

  if (mouseIsPressed) handAnim += 3;
  else handAnim -= 3;
  if (handAnim > 100) handAnim = 100;
  if (handAnim < 0) handAnim = 0;

  if (keyIsDown(79)) mouthAnim += 3;
  else mouthAnim -= 3;
  if (mouthAnim > 100) mouthAnim = 100;
  if (mouthAnim < 0) mouthAnim = 0;

  let handY = 450 - (handAnim * 2.2);
  let lipH = 10 + (mouthAnim * 0.65);
  let grillzH = 2 + (mouthAnim * 0.22);

  fill(20); noStroke();
  ellipse(300, 450, 450, 300);
  fill(255, 235, 215);
  rect(275, 280, 50, 40);
  stroke(0); strokeWeight(1);
  ellipse(300, 180, 200, 240);
  fill(10);
  arc(300, 160, 210, 200, PI, TWO_PI);
  fill(255, 235, 215);
  ellipse(190, 210, 30, 50);
  ellipse(410, 210, 30, 50);
  fill(255);
  ellipse(260, 170, 35, 20);
  ellipse(340, 170, 35, 20);
  fill(0);
  circle(260, 170, 15);
  circle(340, 170, 15);
  noFill(); stroke(150, 100, 50, 100);
  arc(300, 210, 20, 10, 0, PI);

  stroke(0); fill(50); strokeWeight(2);
  let lipY = 235 - (lipH - 10) / 2;
  rect(245, lipY, 110, lipH, 10);

  strokeWeight(1); stroke(0, 50);
  let upperY = lipY + 3;
  let lowerY = lipY + lipH - grillzH - 3;
  for (let i = 0; i < 4; i++) {
    let x = 250 + i * 25;
    fill(220, 230, 240); rect(x, upperY, 25, grillzH);
    noStroke(); fill(255);
    circle(x + 25 * 0.3, upperY + grillzH * 0.3, 4);
    circle(x + 25 * 0.7, upperY + grillzH * 0.7, 2);
    stroke(0, 50); strokeWeight(1);
  }
  for (let i = 0; i < 4; i++) {
    let x = 250 + i * 25;
    fill(220, 230, 240); rect(x, lowerY, 25, grillzH);
    noStroke(); fill(255);
    circle(x + 25 * 0.3, lowerY + grillzH * 0.3, 4);
    circle(x + 25 * 0.7, lowerY + grillzH * 0.7, 2);
    stroke(0, 50); strokeWeight(1);
  }

  if (mouthAnim > 80 && frameCount % 15 < 5) {
    for (let i = 0; i < 4; i++) {
      fill(255); noStroke();
      circle(250 + i * 25 + 12 + random(-5, 5), upperY + random(0, grillzH), random(1, 4));
    }
  }

  fill(255, 220, 190); stroke(0); strokeWeight(1);
  rect(195, handY, 50, 30, 10);
  rect(355, handY, 50, 30, 10);

  fill(200); noStroke();
  circle(188, 200, 8); circle(188, 215, 8); circle(188, 230, 8);
  circle(412, 200, 8); circle(412, 215, 8); circle(412, 230, 8);
}