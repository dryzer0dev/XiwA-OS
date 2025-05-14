from PIL import Image, ImageDraw, ImageFont
import os

def create_logo():
    img = Image.new('RGB', (800, 400), color='black')
    draw = ImageDraw.Draw(img)
    
    draw.rectangle([(50, 50), (750, 350)], outline='cyan', width=3)
    
    try:
        font = ImageFont.truetype("arial.ttf", 72)
    except:
        font = ImageFont.load_default()
    
    text = "XiwA-OS"
    text_bbox = draw.textbbox((0, 0), text, font=font)
    text_width = text_bbox[2] - text_bbox[0]
    text_height = text_bbox[3] - text_bbox[1]
    
    x = (800 - text_width) // 2
    y = (400 - text_height) // 2
    
    for offset in range(3):
        draw.text((x-offset, y), text, font=font, fill='cyan')
        draw.text((x+offset, y), text, font=font, fill='cyan')
        draw.text((x, y-offset), text, font=font, fill='cyan')
        draw.text((x, y+offset), text, font=font, fill='cyan')
    
    draw.text((x, y), text, font=font, fill='white')
    
    os.makedirs('/boot', exist_ok=True)
    
    img.save('/boot/xiwa.png')
    print("Logo généré avec succès dans /boot/xiwa.png")

if __name__ == "__main__":
    create_logo()